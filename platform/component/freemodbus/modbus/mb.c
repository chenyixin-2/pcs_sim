/*
 * FreeModbus Libary: A portable Modbus implementation for Modbus ASCII/RTU.
 * Copyright (c) 2006 Christian Walter <wolti@sil.at>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * File: $Id: mb.c,v 1.28 2010/06/06 13:54:40 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbconfig.h"
#include "mbframe.h"
#include "mbproto.h"
#include "mbfunc.h"
#include "mbctx.h"

#include "mbport.h"
#if MB_RTU_ENABLED == 1
#include "mbrtu.h"
#endif
#if MB_ASCII_ENABLED == 1
#include "mbascii.h"
#endif
#if MB_TCP_ENABLED == 1
#include "mbtcp.h"
#endif

#ifndef MB_PORT_HAS_CLOSE
#define MB_PORT_HAS_CLOSE 0
#endif


/* ----------------------- Static variables ---------------------------------*/

static UCHAR    ucMBAddress;
static eMBMode  eMBCurrentMode;

static enum
{
    STATE_ENABLED,
    STATE_DISABLED,
    STATE_NOT_INITIALIZED
} eMBState = STATE_NOT_INITIALIZED;

/* Functions pointer which are initialized in eMBInit( ). Depending on the
 * mode (RTU or ASCII) the are set to the correct implementations.
 */
static peMBFrameSend peMBFrameSendCur;
static pvMBFrameStart pvMBFrameStartCur;
static pvMBFrameStop pvMBFrameStopCur;
static peMBFrameReceive peMBFrameReceiveCur;
static pvMBFrameClose pvMBFrameCloseCur;

/* Callback functions required by the porting layer. They are called when
 * an external event has happend which includes a timeout or the reception
 * or transmission of a character.
 */
BOOL( *pxMBFrameCBByteReceived ) ( void );
BOOL( *pxMBFrameCBTransmitterEmpty ) ( void );
BOOL( *pxMBPortCBTimerExpired ) ( void );

BOOL( *pxMBFrameCBReceiveFSMCur ) ( void );
BOOL( *pxMBFrameCBTransmitFSMCur ) ( void );

/* An array of Modbus functions handlers which associates Modbus function
 * codes with implementing functions.
 */
static xMBFunctionHandler xFuncHandlers[MB_FUNC_HANDLERS_MAX] = {
#if MB_FUNC_OTHER_REP_SLAVEID_ENABLED > 0
    {MB_FUNC_OTHER_REPORT_SLAVEID, eMBFuncReportSlaveID},
#endif
#if MB_FUNC_READ_INPUT_ENABLED > 0
    {MB_FUNC_READ_INPUT_REGISTER, eMBFuncReadInputRegister},
#endif
#if MB_FUNC_READ_HOLDING_ENABLED > 0
    {MB_FUNC_READ_HOLDING_REGISTER, eMBFuncReadHoldingRegister},
#endif
#if MB_FUNC_WRITE_MULTIPLE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_REGISTERS, eMBFuncWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_WRITE_HOLDING_ENABLED > 0
    {MB_FUNC_WRITE_REGISTER, eMBFuncWriteHoldingRegister},
#endif
#if MB_FUNC_READWRITE_HOLDING_ENABLED > 0
    {MB_FUNC_READWRITE_MULTIPLE_REGISTERS, eMBFuncReadWriteMultipleHoldingRegister},
#endif
#if MB_FUNC_READ_COILS_ENABLED > 0
    {MB_FUNC_READ_COILS, eMBFuncReadCoils},
#endif
#if MB_FUNC_WRITE_COIL_ENABLED > 0
    {MB_FUNC_WRITE_SINGLE_COIL, eMBFuncWriteCoil},
#endif
#if MB_FUNC_WRITE_MULTIPLE_COILS_ENABLED > 0
    {MB_FUNC_WRITE_MULTIPLE_COILS, eMBFuncWriteMultipleCoils},
#endif
#if MB_FUNC_READ_DISCRETE_INPUTS_ENABLED > 0
    {MB_FUNC_READ_DISCRETE_INPUTS, eMBFuncReadDiscreteInputs},
#endif
};

/* ----------------------- Start implementation -----------------------------*/

eMBErrorCode eMBInit(/* OUT */fmodbus_t** pctx, eMBMode eMode, UCHAR ucSlaveAddress, CHAR* szPort, ULONG ulBaudRate, eMBParity eParity )
{
    fmodbus_t* ctx;
    eMBErrorCode    eStatus = MB_ENOERR;

    vMBPortLog( MB_LOG_INFO, "SER-eMBInit", "slave addr:%d, szport:%s\n", ucSlaveAddress, szPort );
    ctx = (fmodbus_t *) malloc(sizeof(fmodbus_t));
    if(ctx == NULL)
    {
        return MB_EIO;
    }

    /*
    ctx->usRegHoldingStart = 0;
    ctx->REG_HOLDING_NREGS = 256;
    ctx->usRegHoldingBuf = (USHORT *) malloc(sizeof(USHORT)*ctx->REG_HOLDING_NREGS);
    if( ctx->usRegHoldingBuf == NULL )
    {
        free(ctx);
        //log_info("usRegHoldingBuf malloc fail!\n");
        return MB_EIO;
    }
    */

    *pctx = ctx;

    /* check preconditions */
    if( ( ucSlaveAddress == MB_ADDRESS_BROADCAST ) ||
        ( ucSlaveAddress < MB_ADDRESS_MIN ) || ( ucSlaveAddress > MB_ADDRESS_MAX ) )
    {
        eStatus = MB_EINVAL;
    }
    else
    {
        ctx->ucMBAddress = ucSlaveAddress;
        ctx->eMode = eMode;

        switch ( eMode )
        {
#if MB_RTU_ENABLED > 0
        case MB_RTU:
            ctx->pvMBFrameStartCur = eMBRTUStart;
            ctx->pvMBFrameStopCur = eMBRTUStop;
            ctx->peMBFrameSendCur = eMBRTUSend;
            ctx->peMBFrameReceiveCur = eMBRTUReceive;
            ctx->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            ctx->pxMBFrameCBByteReceived = xMBRTUReceiveFSM;
            ctx->pxMBFrameCBTransmitterEmpty = xMBRTUTransmitFSM;
            ctx->pxMBPortCBTimerExpired = xMBRTUTimerT35Expired;

            /* ctx->ucPort = ucPort; */
            strcpy(ctx->szPort, szPort);
            ctx->eParity = eParity;
            ctx->ulBaudRate = ulBaudRate;
            ctx->ucDataBits = 8;
            ctx->eMBState = STATE_NOT_INITIALIZED;
            pthread_mutex_init(&ctx->xLock,NULL);
            eStatus = eMBRTUInit( ctx );
            break;
#endif
#if MB_ASCII_ENABLED > 0
        case MB_ASCII:
            pvMBFrameStartCur = eMBASCIIStart;
            pvMBFrameStopCur = eMBASCIIStop;
            peMBFrameSendCur = eMBASCIISend;
            peMBFrameReceiveCur = eMBASCIIReceive;
            pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBPortClose : NULL;
            pxMBFrameCBByteReceived = xMBASCIIReceiveFSM;
            pxMBFrameCBTransmitterEmpty = xMBASCIITransmitFSM;
            pxMBPortCBTimerExpired = xMBASCIITimerT1SExpired;

            eStatus = eMBASCIIInit( ucMBAddress, ucPort, ulBaudRate, eParity );
            break;
#endif
        default:
            eStatus = MB_EINVAL;
        }

        if( eStatus == MB_ENOERR )
        {
            if( !xMBPortEventInit( ctx ) )
            {
                /* port dependent event module initalization failed. */
                eStatus = MB_EPORTERR;
            }
            else
            {
                ctx->eMBCurrentMode = eMode;
                ctx->eMBState = STATE_DISABLED;
            }
        }
    }
    return eStatus;
}

#if MB_TCP_ENABLED > 0
eMBErrorCode
eMBTCPInit(/* OUT */fmodbus_t** pctx, USHORT ucTCPPort )
{
    fmodbus_t* ctx;
    eMBErrorCode    eStatus = MB_ENOERR;

    ctx = (fmodbus_t *) malloc(sizeof(fmodbus_t));
    if(ctx == NULL)
    {
        return MB_EIO;
    }

    *pctx = ctx;
    memset(ctx,0,sizeof(fmodbus_t));

    ctx->xClientSocket = INVALID_SOCKET;

    if( ( eStatus = eMBTCPDoInit(ctx, ucTCPPort ) ) != MB_ENOERR )
    {
        ctx->eMBState = STATE_DISABLED;
    }
    else if( !xMBPortEventInit( ctx ) )
    {
        /* Port dependent event module initalization failed. */
        eStatus = MB_EPORTERR;
    }
    else
    {
        ctx->pvMBFrameStartCur = eMBTCPStart;
        ctx->pvMBFrameStopCur = eMBTCPStop;
        ctx->peMBFrameReceiveCur = eMBTCPReceive;
        ctx->peMBFrameSendCur = eMBTCPSend;
        ctx->pvMBFrameCloseCur = MB_PORT_HAS_CLOSE ? vMBTCPPortClose : NULL;
        ctx->ucMBAddress = MB_TCP_PSEUDO_ADDRESS;
        ctx->eMBCurrentMode = MB_TCP;
        ctx->eMBState = STATE_DISABLED;
    }
    return eStatus;
}
#endif

eMBErrorCode
eMBRegisterCB(fmodbus_t* ctx, UCHAR ucFunctionCode, pxMBFunctionHandler pxHandler )
{
    int             i;
    eMBErrorCode    eStatus;

    if( ( 0 < ucFunctionCode ) && ( ucFunctionCode <= 127 ) )
    {
        ENTER_CRITICAL_SECTION( ctx );
        if( pxHandler != NULL )
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( ( xFuncHandlers[i].pxHandler == NULL ) ||
                    ( xFuncHandlers[i].pxHandler == pxHandler ) )
                {
                    xFuncHandlers[i].ucFunctionCode = ucFunctionCode;
                    xFuncHandlers[i].pxHandler = pxHandler;
                    break;
                }
            }
            eStatus = ( i != MB_FUNC_HANDLERS_MAX ) ? MB_ENOERR : MB_ENORES;
        }
        else
        {
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                if( xFuncHandlers[i].ucFunctionCode == ucFunctionCode )
                {
                    xFuncHandlers[i].ucFunctionCode = 0;
                    xFuncHandlers[i].pxHandler = NULL;
                    break;
                }
            }
            /* Remove can't fail. */
            eStatus = MB_ENOERR;
        }
        EXIT_CRITICAL_SECTION( ctx );
    }
    else
    {
        eStatus = MB_EINVAL;
    }
    return eStatus;
}


eMBErrorCode
eMBClose( fmodbus_t* ctx )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ctx->eMBState == STATE_DISABLED )
    {
        if( ctx->pvMBFrameCloseCur != NULL )
        {
            ctx->pvMBFrameCloseCur( ctx );
        }
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }

    return eStatus;
}

eMBErrorCode
eMBEnable( fmodbus_t* ctx )
{
    eMBErrorCode    eStatus = MB_ENOERR;

    if( ctx->eMBState == STATE_DISABLED )
    {
        /* Activate the protocol stack. */
        ctx->pvMBFrameStartCur( ctx );
        ctx->eMBState = STATE_ENABLED;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBDisable( fmodbus_t* ctx )
{
    eMBErrorCode    eStatus;

    if( ctx->eMBState == STATE_ENABLED )
    {
        ctx->pvMBFrameStopCur( ctx );
        ctx->eMBState = STATE_DISABLED;
        eStatus = MB_ENOERR;
    }
    else if( ctx->eMBState == STATE_DISABLED )
    {
        eStatus = MB_ENOERR;
    }
    else
    {
        eStatus = MB_EILLSTATE;
    }
    return eStatus;
}

eMBErrorCode
eMBPoll( fmodbus_t* ctx )
{
    //static UCHAR   *ucMBFrame;
    //static UCHAR    ucRcvAddress;
    //static UCHAR    ucFunctionCode;
    //static USHORT   usLength;
    //static eMBException eException;

    int             i;
    eMBErrorCode    eStatus = MB_ENOERR;
    eMBEventType    eEvent;

    /* Check if the protocol stack is ready. */
    if( ctx->eMBState != STATE_ENABLED )
    {
        return MB_EILLSTATE;
    }

    /* Check if there is a event available. If not return control to caller.
     * Otherwise we will handle the event. */
    if( xMBPortEventGet(ctx, &eEvent ) == TRUE )
    {
        switch ( eEvent )
        {
        case EV_READY:
            break;

        case EV_FRAME_RECEIVED:
            eStatus = ctx->peMBFrameReceiveCur(ctx, &ctx->ucRcvAddress, &ctx->ucMBFrame, &ctx->usLength );
            if( eStatus == MB_ENOERR )
            {
                /* Check if the frame is for us. If not ignore the frame. */
                if( ( ctx->ucRcvAddress == ctx->ucMBAddress ) || ( ctx->ucRcvAddress == MB_ADDRESS_BROADCAST ) )
                {
                    ( void )xMBPortEventPost(ctx, EV_EXECUTE );
                }
                else
                {
                    //log_info("%d\n",ctx->ucRcvAddress);//dbg wanggao
                }
            }
            break;

        case EV_EXECUTE:
            ctx->ucFunctionCode = ctx->ucMBFrame[MB_PDU_FUNC_OFF];
            ctx->eException = MB_EX_ILLEGAL_FUNCTION;
            for( i = 0; i < MB_FUNC_HANDLERS_MAX; i++ )
            {
                /* No more function handlers registered. Abort. */
                if( xFuncHandlers[i].ucFunctionCode == 0 )
                {
                    break;
                }
                else if( xFuncHandlers[i].ucFunctionCode == ctx->ucFunctionCode )
                {
                    ctx->eException = xFuncHandlers[i].pxHandler(ctx, ctx->ucMBFrame, &ctx->usLength );
                    break;
                }
            }

            /* If the request was not sent to the broadcast address we
             * return a reply. */
            if( ctx->ucRcvAddress != MB_ADDRESS_BROADCAST )
            {
                if( ctx->eException != MB_EX_NONE )
                {
                    /* An exception occured. Build an error frame. */
                    ctx->usLength = 0;
                    ctx->ucMBFrame[ctx->usLength++] = ( UCHAR )( ctx->ucFunctionCode | MB_FUNC_ERROR );
                    ctx->ucMBFrame[ctx->usLength++] = ctx->eException;
                }
                if( ( ctx->eMBCurrentMode == MB_ASCII ) && MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS )
                {
                    vMBPortTimersDelay(ctx, MB_ASCII_TIMEOUT_WAIT_BEFORE_SEND_MS );
                }
                eStatus = ctx->peMBFrameSendCur(ctx, ctx->ucMBAddress, ctx->ucMBFrame, ctx->usLength );
            }
            break;

        case EV_FRAME_SENT:
            break;
        }
    }
    return MB_ENOERR;
}

