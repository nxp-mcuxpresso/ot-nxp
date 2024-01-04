/*
 * Copyright 2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/*${header:start}*/
#include "board.h"
#include "clock_config.h"
#include "pin_mux.h"
/*${header:end}*/

/*${variable:start}*/
/*${variable:end}*/

/*${function:start}*/
void BOARD_InitHardware(void)
{
    BOARD_ConfigMPU();
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
#ifdef BOARD_SPINEL_OVER_SPI
#ifdef BOARD_USE_M2
    BOARD_InitPinsM2();
    BOARD_InitM2SPIPins();
    BOARD_InitM2I2CPins();
#endif
#elif BOARD_SPINEL_OVER_UART
    BOARD_InitArduinoUARTPins();
#endif
#ifdef BOARD_OTW_K32W0_PIN_INIT
    BOARD_InitOTWPins();
#endif
    BOARD_InitDebugConsole();
}
/*${function:end}*/
