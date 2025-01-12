/*
 *  Copyright (c) 2024, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements the OpenThread platform abstraction for Thread Radio Encapsulation Link (TREL) using DNS-SD
 * and UDP/IPv6.
 *
 */

/* -------------------------------------------------------------------------- */
/*                                  Includes                                  */
/* -------------------------------------------------------------------------- */

#include "trel_plat.h"
#include "br_rtos_manager.h"
#include "udp_plat.h"
#include "utils.h"
#include <string.h>
#include <openthread/ip6.h>
#include <openthread/link.h>
#include <openthread/mdns.h>
#include <openthread/tasklet.h>
#include <openthread/trel.h>
#include <openthread/platform/dnssd.h>
#include <openthread/platform/memory.h>
#include <openthread/platform/trel.h>
#include <openthread/platform/udp.h>
#include "common/code_utils.hpp"
#include "config/mle.h"
#include "lwip/api.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "lwip/udp.h"

#include "fsl_component_generic_list.h"
#include "fsl_os_abstraction.h"

/* -------------------------------------------------------------------------- */
/*                                 Definitions                                */
/* -------------------------------------------------------------------------- */

// TXT consists of two entries, ExtAddr and ExtPanID
//  length field + key field + "=" + data
//      LEN                   KEY             =                DATA
//  sizeof(uint8_t) + sizeof("xa") - 1 + sizeof(char) + sizeof(otExtAddress) +
//  sizeof(uint8_t) + sizeof("xp") - 1 + sizeof(char) + sizeof(otExtendedPanId);

#define TXT_DATA_SIZE 24

struct Peer
{
    list_element_t        link;
    const char           *mPeerServiceInstance;
    const char           *mPeerHostName;
    uint8_t               mTxtData[TXT_DATA_SIZE];
    uint8_t               mTxtLength;
    uint16_t              mPort;
    otSockAddr            mSockAddr;
    otMdnsSrvResolver     mSrvResolver;
    otMdnsTxtResolver     mTxtResolver;
    otMdnsAddressResolver mAddrResolver;
};

/* -------------------------------------------------------------------------- */
/*                               Private memory                               */
/* -------------------------------------------------------------------------- */

static otUdpSocket        sTrelSocket;
static otInstance        *sInstance;
static struct netif      *sBackboneNetifPtr;
static otMdnsService      sTrelService;
static bool               sTrelEnabled;
static bool               sBrowsingEnabled;
static otMdnsBrowser      trelBrowser;
static const char         sTrelServiceLabel[] = "_trel._udp";
static uint8_t            sTrelTxtData[TXT_DATA_SIZE];
static otPlatTrelCounters sCounters;
static uint8_t            sPeerNumber;

static list_label_t sPeerList;
static OSA_MUTEX_HANDLE_DEFINE(sMutexHandle);

static const otIp6Address kAnyAddress = {
    .mFields.m8 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/* -------------------------------------------------------------------------- */
/*                             Private prototypes                             */
/* -------------------------------------------------------------------------- */

static void HandleServiceBrowseResult(otInstance *aInstance, const otMdnsBrowseResult *aResult);
static void HandleIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResult *aResolver);
static void HandleServiceTxtResolveResult(otInstance *aInstance, const otMdnsTxtResult *aResult);
static void HandleServiceResolveResult(otInstance *aInstance, const otMdnsSrvResult *aResult);

static bool TrelStartBrowser();
static void TrelSocketReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo);

static void         removeAndFreeInternalPeerListEntry(list_element_handle_t aListElement);
static otError      createAndAppendPeerListEntry(struct Peer aPeer);
static struct Peer *findPeerByServiceInstance(const char *aServiceInstanceName);
static struct Peer *findPeerByHostName(const char *aHostName);
static void         RemoveAllPeersAndNotify();
static void         RemoveTrelServiceInstance(struct Peer *aElement);
static void         AddTrelServiceInstance(const char *aServiceInstanceName);
static void         CheckTrelPeerStorage();
static void         HandleTrelRegistrationCallback(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError);
static void         StopAllPeerResolvers();

/* -------------------------------------------------------------------------- */
/*                              Public functions                              */
/* -------------------------------------------------------------------------- */

void TrelPlatInit(otInstance *aInstance, struct netif *backboneNetif)
{
    sInstance         = aInstance;
    sBackboneNetifPtr = backboneNetif;

    sTrelService.mServiceInstance = CreateBaseName(aInstance, baseServiceInstanceName, true);
    sTrelService.mServiceType     = sTrelServiceLabel;
}

void TrelOnAppReady(const char *aHostName)
{
    // start browsing
    if (!sBrowsingEnabled)
    {
        sBrowsingEnabled = TrelStartBrowser();
    }

    // register TREL service
    sTrelService.mHostName = aHostName;
    otMdnsRegisterService(sInstance, &sTrelService, 0, HandleTrelRegistrationCallback);
}

void TrelOnExternalNetifDown()
{
    // mDNS core stops browsing operation as 'otPlatInfraIfStateChanged` was called before this
    // only change browsing state in this module
    sBrowsingEnabled = false;

    RemoveAllPeersAndNotify();
}

void otPlatTrelEnable(otInstance *aInstance, uint16_t *aUdpPort)
{
    OT_UNUSED_VARIABLE(aInstance);
    LIST_Init(&sPeerList, MAX_PEER_NUMBER);
    if (KOSA_StatusSuccess != OSA_MutexCreate((osa_mutex_handle_t)sMutexHandle))
    {
        assert(true);
    }
    sTrelSocket.mHandler = TrelSocketReceive;

    struct udp_pcb *pcb = NULL;

    VerifyOrExit(!sTrelEnabled);

    VerifyOrExit(otPlatUdpSocket(&sTrelSocket) == OT_ERROR_NONE);
    VerifyOrExit(otPlatUdpBind(&sTrelSocket) == OT_ERROR_NONE);
    VerifyOrExit(otPlatUdpBindToNetif(&sTrelSocket, OT_NETIF_BACKBONE) == OT_ERROR_NONE);

    pcb = (struct udp_pcb *)sTrelSocket.mHandle;

    *aUdpPort                   = pcb->local_port;
    sTrelSocket.mSockName.mPort = pcb->local_port;
    sTrelEnabled                = true;

    if (BrMdnsHostIsInitialized())
    {
        // start browsing
        if (!sBrowsingEnabled)
        {
            sBrowsingEnabled = TrelStartBrowser();
        }

        otMdnsRegisterService(sInstance, &sTrelService, 0, HandleTrelRegistrationCallback);
    }

    otPlatTrelResetCounters(aInstance);
exit:
    return;
}

void otPlatTrelDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    VerifyOrExit(sTrelEnabled);

    // close UDP socket
    otPlatUdpClose(&sTrelSocket);

    // stop browsing
    otMdnsStopBrowser(sInstance, &trelBrowser);
    sBrowsingEnabled = false;

    // unregister service
    (void)otMdnsUnregisterService(sInstance, &sTrelService);

    (void)OSA_MutexDestroy((osa_mutex_handle_t)sMutexHandle);

    sTrelEnabled = false;

    StopAllPeerResolvers();
    RemoveAllPeersAndNotify();

exit:
    return;
}

void otPlatTrelSend(otInstance       *aInstance,
                    const uint8_t    *aUdpPayload,
                    uint16_t          aUdpPayloadLen,
                    const otSockAddr *aDestSockAddr)
{
    OT_UNUSED_VARIABLE(aInstance);

    otMessage    *message;
    otMessageInfo messageInfo;
    memset(&messageInfo, 0, sizeof(otMessageInfo));
    otMessageSettings msgSettings = {.mLinkSecurityEnabled = false, .mPriority = OT_MESSAGE_PRIORITY_NORMAL};

    VerifyOrExit(sTrelEnabled);

    message = otUdpNewMessage(sInstance, &msgSettings);
    VerifyOrExit(message != NULL);
    if (otMessageAppend(message, aUdpPayload, aUdpPayloadLen) != OT_ERROR_NONE)
    {
        otMessageFree(message);
        ++sCounters.mTxFailure;
        return;
    }

    messageInfo.mPeerAddr = aDestSockAddr->mAddress;
    messageInfo.mPeerPort = aDestSockAddr->mPort;
    messageInfo.mSockPort = sTrelSocket.mSockName.mPort;
    messageInfo.mSockAddr = kAnyAddress;

    if (otPlatUdpSend(&sTrelSocket, message, &messageInfo) == OT_ERROR_NONE)
    {
        ++sCounters.mTxPackets;
        sCounters.mTxBytes += aUdpPayloadLen;
    }
    else
    {
        ++sCounters.mTxFailure;
    }

exit:
    return;
}

void otPlatTrelRegisterService(otInstance *aInstance, uint16_t aPort, const uint8_t *aTxtData, uint8_t aTxtLength)
{
    memset(sTrelTxtData, 0, sizeof(sTrelTxtData));
    memcpy(sTrelTxtData, aTxtData, aTxtLength);

    sTrelService.mPort          = aPort;
    sTrelService.mTxtData       = sTrelTxtData;
    sTrelService.mTxtDataLength = aTxtLength;

    if (BrMdnsHostIsInitialized())
    {
        otMdnsRegisterService(aInstance, &sTrelService, 0, HandleTrelRegistrationCallback);
    }
}

const otPlatTrelCounters *otPlatTrelGetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    return &sCounters;
}

void otPlatTrelResetCounters(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    memset(&sCounters, 0, sizeof(sCounters));
}

/* -------------------------------------------------------------------------- */
/*                              Private functions                             */
/* -------------------------------------------------------------------------- */

static bool TrelStartBrowser()
{
    memset(&trelBrowser, 0, sizeof(trelBrowser));

    trelBrowser.mServiceType  = sTrelServiceLabel;
    trelBrowser.mSubTypeLabel = NULL;
    trelBrowser.mInfraIfIndex = netif_get_index(sBackboneNetifPtr);
    trelBrowser.mCallback     = HandleServiceBrowseResult;
    return otMdnsStartBrowser(sInstance, &trelBrowser) == OT_ERROR_NONE ? true : false;
}

static void TrelSocketReceive(void *aContext, otMessage *aMessage, const otMessageInfo *aMessageInfo)
{
    OT_UNUSED_VARIABLE(aContext);

    uint16_t messageLen     = otMessageGetLength(aMessage);
    uint8_t *rxPacketBuffer = (uint8_t *)otPlatCAlloc(1, messageLen);
    otMessageRead(aMessage, 0, rxPacketBuffer, messageLen);
    otMessageFree(aMessage);
    ++sCounters.mRxPackets;
    sCounters.mRxBytes += messageLen;
    otPlatTrelHandleReceived(sInstance, rxPacketBuffer, messageLen);
    otPlatFree(rxPacketBuffer);
}

static void HandleServiceBrowseResult(otInstance *aInstance, const otMdnsBrowseResult *aResult)
{
    VerifyOrExit(sTrelEnabled);
    struct Peer *element = findPeerByServiceInstance(aResult->mServiceInstance);
    if (element)
    {
        if (aResult->mTtl)
        {
            otMdnsStartSrvResolver(sInstance, &(element->mSrvResolver));
        }
        else
        {
            RemoveTrelServiceInstance(element);
        }
    }
    else
    {
        AddTrelServiceInstance(aResult->mServiceInstance);
    }
exit:
    return;
}

static void HandleServiceResolveResult(otInstance *aInstance, const otMdnsSrvResult *aResult)
{
    VerifyOrExit(sTrelEnabled);
    struct Peer *element = findPeerByServiceInstance(aResult->mServiceInstance);
    if (element)
    {
        element->mPeerHostName           = aResult->mHostName;
        element->mAddrResolver.mHostName = aResult->mHostName;
        element->mPort                   = aResult->mPort;
        otMdnsStartTxtResolver(sInstance, &element->mTxtResolver);
    }
exit:
    return;
}

static void HandleServiceTxtResolveResult(otInstance *aInstance, const otMdnsTxtResult *aResult)
{
    VerifyOrExit(sTrelEnabled);
    struct Peer *element = findPeerByServiceInstance(aResult->mServiceInstance);
    if (element)
    {
        memset(element->mTxtData, 0, sizeof(element->mTxtData));
        memcpy(element->mTxtData, aResult->mTxtData, aResult->mTxtDataLength);
        element->mTxtLength = aResult->mTxtDataLength;
        otMdnsStartIp6AddressResolver(sInstance, &element->mAddrResolver);
    }
exit:
    return;
}

static void HandleIp6AddressResolver(otInstance *aInstance, const otMdnsAddressResult *aResult)
{
    VerifyOrExit(sTrelEnabled);
    struct Peer       *element         = findPeerByHostName(aResult->mHostName);
    otIp6Address       selectedAddress = {.mFields = 0};
    otPlatTrelPeerInfo peer;

    if (element)
    {
        otMdnsStopIp6AddressResolver(sInstance, &element->mAddrResolver);

        for (uint8_t i = 0; i < aResult->mAddressesLength; i++)
        {
            if (aResult->mAddresses[i].mTtl &&
                (otIp6IsAddressUnspecified(&selectedAddress) ||
                 (memcmp(selectedAddress.mFields.m8, aResult->mAddresses[i].mAddress.mFields.m8, sizeof(otIp6Address)) <
                  0)))
            {
                selectedAddress = aResult->mAddresses[i].mAddress;
            }
        }
        memcpy(&element->mSockAddr, &selectedAddress, sizeof(peer.mSockAddr.mAddress));

        peer.mRemoved        = false;
        peer.mSockAddr.mPort = element->mPort;
        memcpy(&peer.mSockAddr.mAddress, &element->mSockAddr.mAddress, sizeof(peer.mSockAddr.mAddress));
        peer.mTxtData   = element->mTxtData;
        peer.mTxtLength = element->mTxtLength;

        otPlatTrelHandleDiscoveredPeerInfo(sInstance, &peer);
    }
exit:
    return;
}

static otError createAndAppendPeerListEntry(struct Peer aPeer)
{
    list_status_t result  = kLIST_Ok;
    struct Peer  *newPeer = (struct Peer *)otPlatCAlloc(1, sizeof(struct Peer));
    VerifyOrExit(newPeer != NULL);

    memcpy(newPeer, &aPeer, sizeof(aPeer));

    (void)OSA_MutexLock((osa_mutex_handle_t)sMutexHandle, osaWaitForever_c);
    result = LIST_AddTail(&sPeerList, (list_element_handle_t)newPeer);
    (void)OSA_MutexUnlock((osa_mutex_handle_t)sMutexHandle);

    sPeerNumber++;

exit:
    return (result == kLIST_Ok) ? OT_ERROR_NONE : OT_ERROR_FAILED;
}

static struct Peer *findPeerByServiceInstance(const char *aServiceInstanceName)
{
    list_element_handle_t element = LIST_GetHead(&sPeerList);
    while (element != NULL)
    {
        struct Peer *peer = (struct Peer *)element;
        if (!strcmp(peer->mPeerServiceInstance, aServiceInstanceName))
        {
            return peer;
        }
        element = LIST_GetNext(element);
    }
    return NULL;
}

static struct Peer *findPeerByHostName(const char *aHostName)
{
    list_element_handle_t element = LIST_GetHead(&sPeerList);

    while (element != NULL)
    {
        struct Peer *peer = (struct Peer *)element;
        if (!strcmp(peer->mPeerHostName, aHostName))
        {
            return peer;
        }
        element = LIST_GetNext(element);
    }
    return NULL;
}

static void removeAndFreeInternalPeerListEntry(list_element_handle_t aListElement)
{
    if (sTrelEnabled)
    {
        (void)OSA_MutexLock((osa_mutex_handle_t)sMutexHandle, osaWaitForever_c);
        (void)LIST_RemoveElement(aListElement);
        (void)OSA_MutexUnlock((osa_mutex_handle_t)sMutexHandle);
    }
    else
    {
        (void)LIST_RemoveElement(aListElement);
    }

    otPlatFree(aListElement);
    sPeerNumber--;
}

static void RemoveAllPeersAndNotify()
{
    list_element_handle_t element = LIST_GetHead(&sPeerList);

    while (element != NULL)
    {
        if (otTrelIsEnabled(sInstance))
        {
            struct Peer *peer = (struct Peer *)element;
            // notify OpenThread;
            otPlatTrelPeerInfo peerInfo;
            peerInfo.mRemoved   = true;
            peerInfo.mSockAddr  = peer->mSockAddr;
            peerInfo.mTxtData   = peer->mTxtData;
            peerInfo.mTxtLength = peer->mTxtLength;

            otPlatTrelHandleDiscoveredPeerInfo(sInstance, &peerInfo);
        }
        // remove from internal list
        removeAndFreeInternalPeerListEntry(element);

        element = LIST_GetHead(&sPeerList);
    }
}

static void RemoveTrelServiceInstance(struct Peer *aElement)
{
    otPlatTrelPeerInfo peerInfo;
    peerInfo.mRemoved   = true;
    peerInfo.mSockAddr  = aElement->mSockAddr;
    peerInfo.mTxtData   = aElement->mTxtData;
    peerInfo.mTxtLength = aElement->mTxtLength;

    otPlatTrelHandleDiscoveredPeerInfo(sInstance, &peerInfo);

    otMdnsStopSrvResolver(sInstance, &(aElement->mSrvResolver));
    otMdnsStopTxtResolver(sInstance, &(aElement->mTxtResolver));

    removeAndFreeInternalPeerListEntry((list_element_handle_t)aElement);
}

static void AddTrelServiceInstance(const char *aServiceInstanceName)
{
    struct Peer peer;
    memset(&peer, 0, sizeof(peer));
    peer.mPeerServiceInstance = aServiceInstanceName;

    peer.mSrvResolver.mServiceInstance = aServiceInstanceName;
    peer.mSrvResolver.mServiceType     = sTrelServiceLabel;
    peer.mSrvResolver.mInfraIfIndex    = netif_get_index(sBackboneNetifPtr);
    peer.mSrvResolver.mCallback        = HandleServiceResolveResult;

    peer.mTxtResolver.mServiceInstance = aServiceInstanceName;
    peer.mTxtResolver.mServiceType     = sTrelServiceLabel;
    peer.mTxtResolver.mInfraIfIndex    = netif_get_index(sBackboneNetifPtr);
    peer.mTxtResolver.mCallback        = HandleServiceTxtResolveResult;

    peer.mAddrResolver.mCallback     = HandleIp6AddressResolver;
    peer.mAddrResolver.mInfraIfIndex = netif_get_index(sBackboneNetifPtr);

    if (createAndAppendPeerListEntry(peer) == OT_ERROR_NONE)
    {
        otMdnsStartSrvResolver(sInstance, &peer.mSrvResolver);
    }
}

static void CheckTrelPeerStorage()
{
    VerifyOrExit(sPeerNumber >= MAX_PEER_NUMBER);
exit:
    return;
}

static void HandleTrelRegistrationCallback(otInstance *aInstance, otMdnsRequestId aRequestId, otError aError)
{
    if (aError != OT_ERROR_NONE)
    {
        sTrelService.mServiceInstance = CreateAlternativeBaseName(aInstance, sTrelService.mServiceInstance);
        otMdnsRegisterService(sInstance, &sTrelService, 0, HandleTrelRegistrationCallback);
    }
}

static void StopAllPeerResolvers()
{
    list_element_handle_t element = LIST_GetHead(&sPeerList);
    while (element != NULL)
    {
        struct Peer *peer = (struct Peer *)element;
        otMdnsStopSrvResolver(sInstance, &(peer->mSrvResolver));
        otMdnsStopTxtResolver(sInstance, &(peer->mTxtResolver));
        element = LIST_GetNext(element);
    }
}
