/*
 *  Copyright (c) 2019, The OpenThread Authors.
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
 *   This file implements an entropy source based on TRNG.
 *
 */

#include "openthread/platform/entropy.h"
#include "fsl_device_registers.h"
#include "fsl_os_abstraction.h"
#include "fsl_rng.h"
#include <stdint.h>
#include <stdlib.h>
#include <utils/code_utils.h>

#if defined(USE_RTOS) && (USE_RTOS == 1)
#define mutex_lock OSA_MutexLock
#define mutex_unlock OSA_MutexUnlock
#else
#define mutex_lock(...)
#define mutex_unlock(...)
#endif
#if defined(USE_RTOS) && (USE_RTOS == 1)
static osaMutexId_t trngMutexHandle = NULL;
#endif

void K32WRandomInit(void)
{
    trng_config_t config;
    uint32_t      seed;

#if defined(USE_RTOS) && (USE_RTOS == 1)
    trngMutexHandle = OSA_MutexCreate();
    otEXPECT(NULL != trngMutexHandle);
#endif

    TRNG_GetDefaultConfig(&config);
    config.mode = trng_FreeRunning;

    otEXPECT(TRNG_Init(RNG, &config) == kStatus_Success);

    otEXPECT(TRNG_GetRandomData(RNG, &seed, sizeof(seed)) == kStatus_Success);

    srand(seed);

exit:
    return;
}

otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError status = OT_ERROR_NONE;
#if defined(USE_RTOS) && (USE_RTOS == 1)
    mutex_lock(trngMutexHandle, osaWaitForever_c);
#endif
    otEXPECT_ACTION((aOutput != NULL), status = OT_ERROR_INVALID_ARGS);
    otEXPECT_ACTION(TRNG_GetRandomData(RNG, aOutput, aOutputLength) == kStatus_Success, status = OT_ERROR_FAILED);

exit:
#if defined(USE_RTOS) && (USE_RTOS == 1)
    mutex_unlock(trngMutexHandle);
#endif
    return status;
}
