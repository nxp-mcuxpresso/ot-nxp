/*
 *  Copyright 2020-2023 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _WIFI_CONFIG_H_
#define _WIFI_CONFIG_H_

#define CONFIG_WIFI_MAX_PRIO (configMAX_PRIORITIES - 1)

#ifndef RW610
#define CONFIG_MAX_AP_ENTRIES 10
#else
#define CONFIG_MAX_AP_ENTRIES 30
#endif

#if defined(SD8977) || defined(SD8978) || defined(SD8987) || defined(RW610)
#define CONFIG_5GHz_SUPPORT 1
#endif

#ifndef RW610
#define CONFIG_SDIO_MULTI_PORT_RX_AGGR 1
#endif

#if defined(SD8987) || defined(RW610)
#define CONFIG_11AC
#undef CONFIG_WMM
#endif

#if defined(RW610)
#define PRINTF_FLOAT_ENABLE 1
#define CONFIG_11AX
#undef CONFIG_IMU_GDMA
/* WMM options */
#define CONFIG_WMM
#undef CONFIG_WMM_CERT
#define CONFIG_WMM_ENH
#undef AMSDU_IN_AMPDU
/* OWE mode */
#undef CONFIG_OWE
/* WLAN SCAN OPT */
#define CONFIG_SCAN_WITH_RSSIFILTER
/* WLAN white/black list opt */
#define CONFIG_WIFI_DTIM_PERIOD
#define CONFIG_UART_INTERRUPT
#define CONFIG_WIFI_CAPA
#define CONFIG_WIFI_11D_ENABLE
#define CONFIG_WIFI_HIDDEN_SSID
#define CONFIG_WMM_UAPSD
#define CONFIG_WIFI_GET_LOG
#define CONFIG_ENABLE_802_11K
#define CONFIG_WIFI_TX_PER_TRACK
#define CONFIG_ROAMING
#undef CONFIG_HOST_SLEEP
#define CONFIG_CSI
#define CONFIG_WIFI_RESET
#define CONFIG_NET_MONITOR
#define CONFIG_WIFI_MEM_ACCESS
#define CONFIG_WIFI_REG_ACCESS
#define CONFIG_ECSA
#define CONFIG_WIFI_EU_CRYPTO
#define CONFIG_EXT_SCAN_SUPPORT
#define CONFIG_COMPRESS_TX_PWTBL
// OTP options
#undef OTP_CHANINFO
#define WIFI_ADD_ON 1
#endif

#ifdef CONFIG_11AX
#define CONFIG_11K 1
#define CONFIG_11V 1
#ifndef CONFIG_WPA_SUPP
#define CONFIG_MBO 1
#endif
#endif

#define CONFIG_IPV6               1
#define CONFIG_MAX_IPV6_ADDRESSES 8

/* Logs */
#define CONFIG_ENABLE_ERROR_LOGS   1
#define CONFIG_ENABLE_WARNING_LOGS 1

/* WLCMGR debug */
#undef CONFIG_WLCMGR_DEBUG

/*
 * Wifi extra debug options
 */
#undef CONFIG_WIFI_EXTRA_DEBUG
#undef CONFIG_WIFI_EVENTS_DEBUG
#undef CONFIG_WIFI_CMD_RESP_DEBUG
#undef CONFIG_WIFI_PKT_DEBUG
#undef CONFIG_WIFI_SCAN_DEBUG
#undef CONFIG_WIFI_IO_INFO_DUMP
#undef CONFIG_WIFI_IO_DEBUG
#undef CONFIG_WIFI_IO_DUMP
#undef CONFIG_WIFI_MEM_DEBUG
#undef CONFIG_WIFI_AMPDU_DEBUG
#undef CONFIG_WIFI_TIMER_DEBUG
#undef CONFIG_WIFI_SDIO_DEBUG
#undef CONFIG_WIFI_FW_DEBUG

/*
 * Heap debug options
 */
#undef CONFIG_HEAP_DEBUG
#undef CONFIG_HEAP_STAT

#endif /* _WIFI_CONFIG_H_ */
