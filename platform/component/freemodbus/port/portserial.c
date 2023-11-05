/*
 * FreeModbus Libary: Linux Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.3 2006/10/12 08:35:34 wolti Exp $
 */

/* ----------------------- Standard includes --------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"
#include "mbctx.h"

/* ----------------------- Defines  -----------------------------------------*/
#if MB_ASCII_ENABLED == 1
#define BUF_SIZE    513         /* must hold a complete ASCII frame. */
#else
#define BUF_SIZE    256         /* must hold a complete RTU frame. */
#endif

/* ----------------------- Static variables ---------------------------------*/
static int      iSerialFd = -1;
static BOOL     bRxEnabled;
static BOOL     bTxEnabled;

static ULONG    ulTimeoutMs;
static UCHAR    ucBuffer[BUF_SIZE];
static int      uiRxBufferPos;
static int      uiTxBufferPos;

static struct termios xOldTIO;

/* ----------------------- Function prototypes ------------------------------*/
static BOOL     prvbMBPortSerialRead(fmodbus_t* ctx, UCHAR * pucBuffer, USHORT usNBytes, USHORT * usNBytesRead );
static BOOL     prvbMBPortSerialWrite(fmodbus_t* ctx, UCHAR * pucBuffer, USHORT usNBytes );

/* ----------------------- Begin implementation -----------------------------*/
void
vMBPortSerialEnable(fmodbus_t* ctx, BOOL bEnableRx, BOOL bEnableTx )
{
    /* it is not allowed that both receiver and transmitter are enabled. */
    assert( !bEnableRx || !bEnableTx );

    if( bEnableRx )
    {
        ( void )tcflush( ctx->iSerialFd, TCIFLUSH );
        ctx->uiRxBufferPos = 0;
        ctx->bRxEnabled = TRUE;
    }
    else
    {
        ctx->bRxEnabled = FALSE;
    }
    if( bEnableTx )
    {
        ctx->bTxEnabled = TRUE;
        ctx->uiTxBufferPos = 0;
    }
    else
    {
        ctx->bTxEnabled = FALSE;
    }
}

//( UCHAR ucPort, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
BOOL
xMBPortSerialInit( fmodbus_t* ctx )
{
    CHAR            szDevice[32];
    BOOL            bStatus = TRUE;

    struct termios  xNewTIO;
    speed_t         xNewSpeed;

    snprintf( szDevice, 32, "%s", ctx->szPort );

    if( ( ctx->iSerialFd = open( szDevice, O_RDWR | O_NOCTTY ) ) < 0 ){
        vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't open serial port %s: %s\n", szDevice, strerror( errno ) );
    }else if( tcgetattr( ctx->iSerialFd, &(ctx->xOldTIO) ) != 0 ){
        vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't get settings from port %s: %s\n", szDevice, strerror( errno ) );
    }else{
        vMBPortLog( MB_LOG_INFO, "SER-INIT", "init ok %s\n", szDevice );

        bzero( &xNewTIO, sizeof( struct termios ) );

        xNewTIO.c_iflag |= IGNBRK | INPCK;
        xNewTIO.c_cflag |= CREAD | CLOCAL;
        switch ( ctx->eParity )
        {
        case MB_PAR_NONE:
            break;
        case MB_PAR_EVEN:
            xNewTIO.c_cflag |= PARENB;
            break;
        case MB_PAR_ODD:
            xNewTIO.c_cflag |= PARENB | PARODD;
            break;
        default:
            bStatus = FALSE;
        }
        switch ( ctx->ucDataBits )
        {
        case 8:
            xNewTIO.c_cflag |= CS8;
            break;
        case 7:
            xNewTIO.c_cflag |= CS7;
            break;
        default:
            bStatus = FALSE;
        }
        switch ( ctx->ulBaudRate )
        {
        case 9600:
            xNewSpeed = B9600;
            break;
        case 19200:
            xNewSpeed = B19200;
            break;
        case 38400:
            xNewSpeed = B38400;
            break;
        case 57600:
            xNewSpeed = B57600;
            break;
        case 115200:
            xNewSpeed = B115200;
            break;
        default:
            bStatus = FALSE;
        }
        if( bStatus )
        {
            if( cfsetispeed( &xNewTIO, xNewSpeed ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set baud rate %ld for port %s: %s\n",
                            ctx->ulBaudRate, strerror( errno ) );
            }
            else if( cfsetospeed( &xNewTIO, xNewSpeed ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set baud rate %ld for port %s: %s\n",
                            ctx->ulBaudRate, szDevice, strerror( errno ) );
            }
            else if( tcsetattr( ctx->iSerialFd, TCSANOW, &xNewTIO ) != 0 )
            {
                vMBPortLog( MB_LOG_ERROR, "SER-INIT", "Can't set settings for port %s: %s\n",
                            szDevice, strerror( errno ) );
            }
            else
            {
                vMBPortSerialEnable(ctx, FALSE, FALSE );
                bStatus = TRUE;
            }
        }
    }
    return bStatus;
}

BOOL
xMBPortSerialSetTimeout(fmodbus_t* ctx, ULONG ulNewTimeoutMs )
{
    if( ulNewTimeoutMs > 0 )
    {
        ctx->ulTimeoutMs = ulNewTimeoutMs;
    }
    else
    {
        ctx->ulTimeoutMs = 1;
    }
    return TRUE;
}

void
vMBPortClose( fmodbus_t* ctx )
{
    if( ctx->iSerialFd != -1 )
    {
        ( void )tcsetattr( ctx->iSerialFd, TCSANOW, &ctx->xOldTIO );
        ( void )close( ctx->iSerialFd );
        ctx->iSerialFd = -1;
    }
}

BOOL
prvbMBPortSerialRead(fmodbus_t* ctx, UCHAR * pucBuffer, USHORT usNBytes, USHORT * usNBytesRead )
{
    BOOL            bResult = TRUE;
    ssize_t         res;
    fd_set          rfds;
    struct timeval  tv;

    tv.tv_sec = 0;
    tv.tv_usec = 50000;
    FD_ZERO( &rfds );
    FD_SET( ctx->iSerialFd, &rfds );

    /* Wait until character received or timeout. Recover in case of an
     * interrupted read system call. */
    do
    {
        if( select( ctx->iSerialFd + 1, &rfds, NULL, NULL, &tv ) == -1 )
        {
            if( errno != EINTR )
            {
                bResult = FALSE;
            }
        }
        else if( FD_ISSET( ctx->iSerialFd, &rfds ) )
        {
            if( ( res = read( ctx->iSerialFd, pucBuffer, usNBytes ) ) == -1 )
            {
                bResult = FALSE;
            }
            else
            {
                *usNBytesRead = ( USHORT ) res;
                break;
            }
        }
        else
        {
            *usNBytesRead = 0;
            break;
        }
    }
    while( bResult == TRUE );
    return bResult;
}

BOOL
prvbMBPortSerialWrite(fmodbus_t* ctx, UCHAR * pucBuffer, USHORT usNBytes )
{
    ssize_t         res;
    size_t          left = ( size_t ) usNBytes;
    size_t          done = 0;

    while( left > 0 )
    {
        if( ( res = write( ctx->iSerialFd, pucBuffer + done, left ) ) == -1 )
        {
            if( errno != EINTR )
            {
                break;
            }
            /* call write again because of interrupted system call. */
            continue;
        }
        done += res;
        left -= res;
    }
    return left == 0 ? TRUE : FALSE;
}

BOOL
xMBPortSerialPoll( fmodbus_t* ctx )
{
    BOOL            bStatus = TRUE;
    USHORT          usBytesRead;
    int             i;

    //printf("a %d %d\n",ctx->bRxEnabled,ctx->bTxEnabled);//dbg wanggao

    while( ctx->bRxEnabled )
    {
        if( prvbMBPortSerialRead(ctx, &(ctx->ucBuffer[0]), BUF_SIZE, &usBytesRead ) )
        {
            if( usBytesRead == 0 )
            {
                /* timeout with no bytes. */
                break;
            }
            else if( usBytesRead > 0 )
            {
                for( i = 0; i < usBytesRead; i++ )
                {
                    /* Call the modbus stack and let him fill the buffers. */
                    ( void )ctx->pxMBFrameCBByteReceived( ctx );
                }
                ctx->uiRxBufferPos = 0;
            }
        }
        else
        {
            vMBPortLog( MB_LOG_ERROR, "SER-POLL", "read failed on serial device: %s\n",
                        strerror( errno ) );
            bStatus = FALSE;
        }
    }
    //printf("b %d %d\n",ctx->bRxEnabled,ctx->bTxEnabled);//dbg wanggao
    if( ctx->bTxEnabled )
    {
        //printf("c %d %d\n",ctx->bRxEnabled,ctx->bTxEnabled);//dbg wanggao
        while( ctx->bTxEnabled )
        {
            ( void )ctx->pxMBFrameCBTransmitterEmpty( ctx );
            /* Call the modbus stack to let him fill the buffer. */
        }
        if( !prvbMBPortSerialWrite(ctx, &ctx->ucBuffer[0], ctx->uiTxBufferPos ) )
        {
            vMBPortLog( MB_LOG_ERROR, "SER-POLL", "write failed on serial device: %s\n",
                        strerror( errno ) );
            bStatus = FALSE;
        }
    }

    return bStatus;
}

BOOL
xMBPortSerialPutByte(fmodbus_t* ctx, CHAR ucByte )
{
    assert( ctx->uiTxBufferPos < BUF_SIZE );
    ctx->ucBuffer[ctx->uiTxBufferPos] = ucByte;
    ctx->uiTxBufferPos++;
    return TRUE;
}

BOOL
xMBPortSerialGetByte(fmodbus_t* ctx, CHAR * pucByte )
{
    assert( ctx->uiRxBufferPos < BUF_SIZE );
    *pucByte = ctx->ucBuffer[ctx->uiRxBufferPos];
    ctx->uiRxBufferPos++;
    return TRUE;
}
