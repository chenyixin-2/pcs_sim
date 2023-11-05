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
 * File: $Id: mbrtu.c,v 1.18 2007/09/12 10:15:56 wolti Exp $
 */

/* ----------------------- System includes ----------------------------------*/
#include "stdlib.h"
#include "string.h"

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbrtu.h"
#include "mbframe.h"

#include "mbcrc.h"
#include "mbport.h"
#include "mbctx.h"

/* ----------------------- Defines ------------------------------------------*/
#define MB_SER_PDU_SIZE_MIN     4       /*!< Minimum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_MAX     256     /*!< Maximum size of a Modbus RTU frame. */
#define MB_SER_PDU_SIZE_CRC     2       /*!< Size of CRC field in PDU. */
#define MB_SER_PDU_ADDR_OFF     0       /*!< Offset of slave address in Ser-PDU. */
#define MB_SER_PDU_PDU_OFF      1       /*!< Offset of Modbus-PDU in Ser-PDU. */

/* ----------------------- Type definitions ---------------------------------*/


/* ----------------------- Static variables ---------------------------------*/
static volatile eMBSndState eSndState;
static volatile eMBRcvState eRcvState;

volatile UCHAR  ucRTUBuf[MB_SER_PDU_SIZE_MAX];

static volatile UCHAR *pucSndBufferCur;
static volatile USHORT usSndBufferCount;

static volatile USHORT usRcvBufferPos;

/* ----------------------- Start implementation -----------------------------*/

//( UCHAR ucSlaveAddress, UCHAR ucPort, ULONG ulBaudRate, eMBParity eParity )
eMBErrorCode
eMBRTUInit( fmodbus_t* ctx )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    ULONG           usTimerT35_50us;

    //printf("eMBRTUInit enter,port:%d\n",ctx->ucPort);//dbg wanggao

    ( void )ctx->ucMBAddress;
    ENTER_CRITICAL_SECTION( ctx );

    /* Modbus RTU uses 8 Databits. */
    if( xMBPortSerialInit( ctx ) != TRUE )
    {
        eStatus = MB_EPORTERR;
    }
    else
    {
        /* If baudrate > 19200 then we should use the fixed timer values
         * t35 = 1750us. Otherwise t35 must be 3.5 times the character time.
         */
        if( ctx->ulBaudRate > 19200 )
        {
            usTimerT35_50us = 35;       /* 1800us. */
        }
        else
        {
            /* The timer reload value for a character is given by:
             *
             * ChTimeValue = Ticks_per_1s / ( Baudrate / 11 )
             *             = 11 * Ticks_per_1s / Baudrate
             *             = 220000 / Baudrate
             * The reload for t3.5 is 1.5 times this value and similary
             * for t3.5.
             */
            usTimerT35_50us = ( 7UL * 220000UL ) / ( 2UL * ctx->ulBaudRate );
        }
        if( xMBPortTimersInit(ctx, ( USHORT ) usTimerT35_50us ) != TRUE )
        {
            eStatus = MB_EPORTERR;
        }
    }
    EXIT_CRITICAL_SECTION( ctx );

    return eStatus;
}

void
eMBRTUStart( fmodbus_t* ctx )
{
    ENTER_CRITICAL_SECTION( ctx );
    /* Initially the receiver is in the state STATE_RX_INIT. we start
     * the timer and if no character is received within t3.5 we change
     * to STATE_RX_IDLE. This makes sure that we delay startup of the
     * modbus protocol stack until the bus is free.
     */
    ctx->eRcvState = STATE_RX_INIT;
    //printf("ctx->eRcvState = STATE_RX_INIT\n");//dbg wanggao
    vMBPortSerialEnable(ctx, TRUE, FALSE );
    vMBPortTimersEnable( ctx );

    EXIT_CRITICAL_SECTION( ctx );
}

void
eMBRTUStop( fmodbus_t* ctx )
{
    ENTER_CRITICAL_SECTION( ctx );
    vMBPortSerialEnable(ctx, FALSE, FALSE );
    vMBPortTimersDisable( ctx );
    EXIT_CRITICAL_SECTION( ctx );
}

eMBErrorCode
eMBRTUReceive(fmodbus_t* ctx, UCHAR * pucRcvAddress, UCHAR ** pucFrame, USHORT * pusLength )
{
    BOOL            xFrameReceived = FALSE;
    eMBErrorCode    eStatus = MB_ENOERR;

    ENTER_CRITICAL_SECTION( ctx );
    assert( ctx->usRcvBufferPos < MB_SER_PDU_SIZE_MAX );

    /* Length and CRC check */
    if( ( ctx->usRcvBufferPos >= MB_SER_PDU_SIZE_MIN )
        && ( usMBCRC16( ( UCHAR * ) ctx->ucRTUBuf, ctx->usRcvBufferPos ) == 0 ) )
    {
        /* Save the address field. All frames are passed to the upper layed
         * and the decision if a frame is used is done there.
         */
        *pucRcvAddress = ctx->ucRTUBuf[MB_SER_PDU_ADDR_OFF];

        /* Total length of Modbus-PDU is Modbus-Serial-Line-PDU minus
         * size of address field and CRC checksum.
         */
        *pusLength = ( USHORT )( ctx->usRcvBufferPos - MB_SER_PDU_PDU_OFF - MB_SER_PDU_SIZE_CRC );

        /* Return the start of the Modbus PDU to the caller. */
        *pucFrame = ( UCHAR * ) & ctx->ucRTUBuf[MB_SER_PDU_PDU_OFF];
        xFrameReceived = TRUE;
    }
    else
    {
        eStatus = MB_EIO;
    }

    EXIT_CRITICAL_SECTION( ctx );
    return eStatus;
}

eMBErrorCode
eMBRTUSend(fmodbus_t* ctx, UCHAR ucSlaveAddress, const UCHAR * pucFrame, USHORT usLength )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    USHORT          usCRC16;

    ENTER_CRITICAL_SECTION( ctx );

    /* Check if the receiver is still in idle state. If not we where to
     * slow with processing the received frame and the master sent another
     * frame on the network. We have to abort sending the frame.
     */
    if( ctx->eRcvState == STATE_RX_IDLE )
    {
        /* First byte before the Modbus-PDU is the slave address. */
        ctx->pucSndBufferCur = ( UCHAR * ) pucFrame - 1;
        ctx->usSndBufferCount = 1;

        /* Now copy the Modbus-PDU into the Modbus-Serial-Line-PDU. */
        ctx->pucSndBufferCur[MB_SER_PDU_ADDR_OFF] = ucSlaveAddress;
        ctx->usSndBufferCount += usLength;

        /* Calculate CRC16 checksum for Modbus-Serial-Line-PDU. */
        usCRC16 = usMBCRC16( ( UCHAR * ) ctx->pucSndBufferCur, ctx->usSndBufferCount );
        ctx->ucRTUBuf[ctx->usSndBufferCount++] = ( UCHAR )( usCRC16 & 0xFF );
        ctx->ucRTUBuf[ctx->usSndBufferCount++] = ( UCHAR )( usCRC16 >> 8 );

        /* Activate the transmitter. */
        ctx->eSndState = STATE_TX_XMIT;
        vMBPortSerialEnable(ctx, FALSE, TRUE );
    }
    else
    {
        eStatus = MB_EIO;
    }
    EXIT_CRITICAL_SECTION( ctx );
    return eStatus;
}

BOOL
xMBRTUReceiveFSM( fmodbus_t* ctx )
{
    BOOL            xTaskNeedSwitch = FALSE;
    UCHAR           ucByte;

    //printf("xMBRTUReceiveFSM\n");//dbg wanggao

    assert( ctx->eSndState == STATE_TX_IDLE );

    /* Always read the character. */
    ( void )xMBPortSerialGetByte(ctx, ( CHAR * ) & ucByte );

    switch ( ctx->eRcvState )
    {
        /* If we have received a character in the init state we have to
         * wait until the frame is finished.
         */
    case STATE_RX_INIT:
        vMBPortTimersEnable( ctx );
        break;

        /* In the error state we wait until all characters in the
         * damaged frame are transmitted.
         */
    case STATE_RX_ERROR:
        vMBPortTimersEnable( ctx );
        break;

        /* In the idle state we wait for a new character. If a character
         * is received the t1.5 and t3.5 timers are started and the
         * receiver is in the state STATE_RX_RECEIVCE.
         */
    case STATE_RX_IDLE:
        ctx->usRcvBufferPos = 0;
        ctx->ucRTUBuf[ctx->usRcvBufferPos++] = ucByte;
        ctx->eRcvState = STATE_RX_RCV;

        /* Enable t3.5 timers. */
        vMBPortTimersEnable( ctx );
        break;

        /* We are currently receiving a frame. Reset the timer after
         * every character received. If more than the maximum possible
         * number of bytes in a modbus frame is received the frame is
         * ignored.
         */
    case STATE_RX_RCV:
        if( ctx->usRcvBufferPos < MB_SER_PDU_SIZE_MAX )
        {
            ctx->ucRTUBuf[ctx->usRcvBufferPos++] = ucByte;
        }
        else
        {
            ctx->eRcvState = STATE_RX_ERROR;
        }
        vMBPortTimersEnable( ctx );
        break;
    }
    return xTaskNeedSwitch;
}

BOOL
xMBRTUTransmitFSM( fmodbus_t* ctx )
{
    BOOL            xNeedPoll = FALSE;

    //printf("xMBRTUTransmitFSM\n");//dbg wanggao

    assert( ctx->eRcvState == STATE_RX_IDLE );

    switch ( ctx->eSndState )
    {
        /* We should not get a transmitter event if the transmitter is in
         * idle state.  */
    case STATE_TX_IDLE:
        /* enable receiver/disable transmitter. */
        vMBPortSerialEnable(ctx, TRUE, FALSE );
        break;

    case STATE_TX_XMIT:
        /* check if we are finished. */
        if( ctx->usSndBufferCount != 0 )
        {
            xMBPortSerialPutByte(ctx, ( CHAR )*ctx->pucSndBufferCur );
            ctx->pucSndBufferCur++;  /* next byte in sendbuffer. */
            ctx->usSndBufferCount--;
        }
        else
        {
            xNeedPoll = xMBPortEventPost(ctx, EV_FRAME_SENT );
            /* Disable transmitter. This prevents another transmit buffer
             * empty interrupt. */
            vMBPortSerialEnable(ctx, TRUE, FALSE );
            ctx->eSndState = STATE_TX_IDLE;
        }
        break;
    }

    return xNeedPoll;
}

BOOL
xMBRTUTimerT35Expired( fmodbus_t* ctx )
{
    BOOL            xNeedPoll = FALSE;

    //printf("xMBRTUTimerT35Expired %d\n",ctx->eRcvState);//dbg wanggao

    switch ( ctx->eRcvState )
    {
        /* Timer t35 expired. Startup phase is finished. */
    case STATE_RX_INIT:
        xNeedPoll = xMBPortEventPost(ctx, EV_READY );
        break;

        /* A frame was received and t35 expired. Notify the listener that
         * a new frame was received. */
    case STATE_RX_RCV:
        xNeedPoll = xMBPortEventPost(ctx, EV_FRAME_RECEIVED );
        break;

        /* An error occured while receiving the frame. */
    case STATE_RX_ERROR:
        break;

        /* Function called in an illegal state. */
    default:
        assert( ( ctx->eRcvState == STATE_RX_INIT ) ||
                ( ctx->eRcvState == STATE_RX_RCV ) || ( ctx->eRcvState == STATE_RX_ERROR ) );
    }

    vMBPortTimersDisable( ctx );
    ctx->eRcvState = STATE_RX_IDLE;

    return xNeedPoll;
}
