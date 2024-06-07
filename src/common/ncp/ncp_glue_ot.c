/* @file ncp_glue_ot.c
 *
 *  @brief This file contains ot ncp command functions.
 *
 *  Copyright 2008-2024 NXP
 *
 *  Licensed under the LA_OPT_NXP_Software_License.txt (the "Agreement")
 */

#include "board.h"
#include "fsl_debug_console.h"
#include "ncp_ot.h"
#include "openthread-system.h"
#include "ot_platform_common.h"
#include "otopcode_private.h"
#include <stdio.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------- */
/*                                Definitions                                 */
/* -------------------------------------------------------------------------- */

#define CMD_SUBCLASS_15D4_LEN (sizeof(cmd_subclass_15D4) / sizeof(struct cmd_subclass_t))

/* -------------------------------------------------------------------------- */
/*                                 Prototypes                                 */
/* -------------------------------------------------------------------------- */

extern void Ot_Data_RxDone(void);
extern void Ot_Data_TxDone(void);

/* -------------------------------------------------------------------------- */
/*                                 Variables                                  */
/* -------------------------------------------------------------------------- */

static uint8_t otNcpCmdFlag = NCP_COMMAND_NOT_READY;

uint8_t rspNcpBuffer[NCP_INBUF_SIZE];

uint8_t  otCurrentCmd[OT_COMMANDS_MAX_LEN];
uint32_t otCmdTotalLengh;

/* -------------------------------------------------------------------------- */
/*                                 Functions                                  */
/* -------------------------------------------------------------------------- */

NCPCmd_DS_COMMAND *ncp_get_ot_response_buffer()
{
    return (NCPCmd_DS_COMMAND *)(rspNcpBuffer);
}

ncp_status_t ot_send_response(uint32_t cmd, uint8_t status, uint8_t *data, size_t len)
{
    NCPCmd_DS_COMMAND *cmd_res = ncp_get_ot_response_buffer();

    cmd_res->header.cmd    = cmd;
    cmd_res->header.size   = NCP_CMD_HEADER_LEN + len;
    cmd_res->header.seqnum = 0x00;
    cmd_res->header.rsvd   = 0;
    cmd_res->header.result = status;

    if (data != NULL)
    {
        memcpy((uint8_t *)cmd_res + NCP_CMD_HEADER_LEN, data, len);
    }

    ncp_tlv_send((void *)cmd_res, cmd_res->header.size);

    return NCP_STATUS_SUCCESS;
}

void ot_ncp_clear_cmd_ready(void)
{
    otNcpCmdFlag = NCP_COMMAND_NOT_READY;
}

ncp_cmd_status ot_ncp_check_cmd_ready(void)
{
    return otNcpCmdFlag;
}

void ot_ncp_copy_cmd_buff(uint8_t *src, uint16_t *pLen)
{
    assert(src != NULL);
    assert(pLen != NULL);

    *pLen = otCmdTotalLengh;

    memcpy(src, otCurrentCmd, *pLen);
}

static void ot_ncp_set_cmd_ready(void)
{
    otNcpCmdFlag = NCP_COMMAND_READY;
}

static int ot_ncp_cmd_handle(void *cmd)
{
    uint16_t cmdLen      = 0;
    uint16_t cmdParamLen = 0;
    char    *pOtCmd      = NULL;

    uint8_t *pCmdParam = (uint8_t *)cmd + 1;
    uint8_t  opCodeIdx = *(((uint8_t *)cmd) + 0); // get ot opcode
    uint8_t *p         = pCmdParam;

    /* Before executed this function, the ncp tlv header has been parsed successfully.
     * The tlv payload part should be an OT standard command, so the exception handling
     * of not support command is subject to the status returned by the OT stack.
     */
    do
    {
        cmdParamLen++;
    } while (*p++ != 0x0D);

    if (opCodeIdx >= (sizeof(otcommands) / sizeof(otcommands[0])))
    {
        // if not find the ot command string, regard ot opcode as ot invalid command
        pOtCmd = (char *)cmd;
        cmdLen = 1;
    }
    else
    {
        // get the real ot command string and length
        pOtCmd = otcommands[opCodeIdx];
        cmdLen = strlen(otcommands[opCodeIdx]);
    }

    otCmdTotalLengh = cmdLen + cmdParamLen;

    if (otCmdTotalLengh > OT_COMMANDS_MAX_LEN)
    {
        OT_PLAT_ERR("NCP command body is too long\r\n");
        goto fail;
    }

    // copy ot command string
    memcpy(otCurrentCmd, pOtCmd, cmdLen);

    // ot command parameters should be appended
    memcpy((uint8_t *)&otCurrentCmd[0] + cmdLen, pCmdParam, cmdParamLen);

    // Notify ot task to process command
    ot_ncp_set_cmd_ready();
    Ot_Data_RxDone();

    return NCP_STATUS_SUCCESS;

fail:
    return NCP_STATUS_ERROR;
}

static int ot_error_ack(void *tlv)
{
    return ot_send_response(NCP_CMD_INVALID_CMD, NCP_CMD_RESULT_ERROR, NULL, 0);
}

struct cmd_t error_ack_cmd = {0, "lookup cmd fail", ot_error_ack, CMD_SYNC};

struct cmd_t ot_command_forward[] = {
    {NCP_OT_CMD_FORWARD, "ot-command-forward", ot_ncp_cmd_handle, CMD_SYNC},
    {NCP_CMD_INVALID, NULL, NULL, NULL},
};

/* Need to define the unused wlan/wifi/system ncp subclass as weak,
 * refer to the wlan/wifi/system subclass in the ncp_glue_common.c
 * file in the SDK, avoid compilation errors.
 */
OT_TOOL_WEAK struct cmd_subclass_t cmd_subclass_wlan[] = {
    {NCP_CMD_INVALID, NULL},
};

OT_TOOL_WEAK struct cmd_subclass_t cmd_subclass_ble[] = {
    {NCP_CMD_INVALID, NULL},
};

struct cmd_subclass_t cmd_subclass_15D4[] = {
    {NCP_15d4_CMD_FORWARD, ot_command_forward},
    {NCP_CMD_INVALID, NULL},
};

OT_TOOL_WEAK struct cmd_subclass_t cmd_subclass_system[] = {
    {NCP_CMD_INVALID, NULL},
};
