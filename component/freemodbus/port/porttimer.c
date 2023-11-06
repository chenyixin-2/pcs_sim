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
 * File: $Id: porttimer.c,v 1.1 2006/08/01 20:58:50 wolti Exp $
 */

/* ----------------------- Standard includes --------------------------------*/
#include <stdlib.h>
#include <sys/time.h>

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbctx.h"

/* ----------------------- Defines ------------------------------------------*/

/* ----------------------- Static variables ---------------------------------*/
ULONG           ulTimeOut;
BOOL            bTimeoutEnable;

static struct timeval xTimeLast;

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit(fmodbus_t* ctx, USHORT usTim1Timerout50us )
{
    ctx->ulTimeOut = usTim1Timerout50us / 20U;
    if( ctx->ulTimeOut == 0 )
        ctx->ulTimeOut = 1;

    return xMBPortSerialSetTimeout(ctx, ctx->ulTimeOut );
}

void
xMBPortTimersClose( fmodbus_t* ctx )
{
    /* Does not use any hardware resources. */
}

void
vMBPortTimerPoll( fmodbus_t* ctx )
{
    ULONG           ulDeltaMS;
    struct timeval  xTimeCur;

    //printf("vMBPortTimerPoll enter\n");//dbg wanggao

    /* Timers are called from the serial layer because we have no high
     * res timer in Win32. */
    if( ctx->bTimeoutEnable )
    {
        if( gettimeofday( &xTimeCur, NULL ) != 0 )
        {
            /* gettimeofday failed - retry next time. */
        }
        else
        {
            ulDeltaMS = ( xTimeCur.tv_sec - ctx->xTimeLast.tv_sec ) * 1000L +
                ( xTimeCur.tv_usec - ctx->xTimeLast.tv_usec ) * 1000L;
            if( ulDeltaMS > ctx->ulTimeOut )
            {
                ctx->bTimeoutEnable = FALSE;
                //printf("expired \n");//dbg wanggao
                ( void )ctx->pxMBPortCBTimerExpired( ctx );
            }
        }
    }
}

void
vMBPortTimersEnable( fmodbus_t* ctx )
{
    int             res = gettimeofday( &ctx->xTimeLast, NULL );

    assert( res == 0 );
    ctx->bTimeoutEnable = TRUE;
}

void
vMBPortTimersDisable( fmodbus_t* ctx )
{
    ctx->bTimeoutEnable = FALSE;
}
