# Copyright (c) 2022-2023, NXP.
# All rights reserved.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the
# names of its contributors may be used to endorse or promote products
# derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

set(PLATFORM_C_FLAGS "-mcpu=cortex-m33+nodsp -mfloat-abi=hard -mthumb -mfpu=fpv5-sp-d16")
set(PLATFORM_LINKER_FLAGS "${PLATFORM_C_FLAGS} -u flexspi_config")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PLATFORM_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PLATFORM_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${PLATFORM_C_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${PLATFORM_LINKER_FLAGS} ")

# FreeRTOS CMake config
set(FREERTOS_PORT GCC_ARM_CM33_NTZ_NONSECURE CACHE STRING "")
set(FREERTOS_HEAP 4)

# Connectivity Framework CMake config
set(CONNFWK_PLATFORM "rw61x" CACHE STRING "")
set(CONNFWK_FLIB ON CACHE BOOL "")
set(CONNFWK_FSABSTRACTION "LFS" CACHE STRING "")
set(CONNFWK_PWR ON CACHE BOOL "")
set(CONNFWK_PWR_SYSTICKS ON CACHE BOOL "")
set(CONNFWK_IGNORE_PLATFORM_SENSORS ON CACHE BOOL "")

# OpenThread config
set(BUILD_TESTING OFF CACHE BOOL "")
set(OT_PLATFORM "external" CACHE STRING "")
set(OT_SLAAC ON CACHE BOOL "")
set(OT_SERVICE ON CACHE BOOL "")
set(OT_PING_SENDER ON CACHE BOOL "")
set(OT_EXTERNAL_HEAP ON CACHE BOOL "")
set(OT_RCP OFF CACHE BOOL "")
set(OT_MTD OFF CACHE BOOL "")
set(OT_APP_CLI OFF CACHE BOOL "")
set(OT_APP_NCP OFF CACHE BOOL "")
set(OT_APP_RCP OFF CACHE BOOL "")
set(OT_APP_RCP OFF CACHE BOOL "")
set(OT_RCP_RESTORATION_MAX_COUNT "10" CACHE STRING "")
set(OT_COMPILE_WARNING_AS_ERROR ON CACHE BOOL "")
set(OT_EXTERNAL_MBEDTLS "nxp-rw612-mbedtls" CACHE STRING "")

# ot-nxp config
set(OT_NXP_PLATFORM_FAMILY "rw" CACHE STRING "")
set(OT_NXP_BOARD "rdrw612bga" CACHE STRING "")
set(OT_NXP_MBEDTLS_PORT "els_pkc" CACHE STRING "")
set(OT_NXP_LWIP ON CACHE BOOL "")
set(OT_NXP_LWIP_IPERF ON CACHE BOOL "")
set(OT_NXP_LWIP_ETH OFF CACHE BOOL "")
set(OT_NXP_LWIP_WIFI OFF CACHE BOOL "")
set(OT_APP_BR_FREERTOS OFF CACHE BOOL "")
set(OT_APP_CLI_FREERTOS ON CACHE BOOL "")
set(OT_APP_CLI_FREERTOS_LOWPOWER ON CACHE BOOL "")
set(OT_APP_CLI_FREERTOS_IPERF OFF CACHE BOOL "")
set(OT_APP_CLI_FREERTOS_PLATFORM ON CACHE BOOL "")
set(OT_NXP_EXPORT_TO_BIN ON CACHE BOOL "")
set(OT_NXP_INDEPENDENT_RESET ON CACHE BOOL "")

# OpenThread stack logs
# set(OT_STACK_ENABLE_LOG ON CACHE BOOL "")
# set(OT_LOG_LEVEL "INFO" CACHE STRING "")
# set(OT_LOG_OUTPUT "PLATFORM_DEFINED" CACHE STRING "")
