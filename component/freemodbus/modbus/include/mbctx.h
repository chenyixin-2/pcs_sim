#ifndef MBCTX_H
#define MBCTX_H

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "mbframe.h"
#include <sys/time.h>
#include <termios.h>


typedef struct _fmodbus_t fmodbus_t;

typedef struct _fmodbus_t
{
    int chanidx;
    int mbsidx;
    eMBMode eMode;
    eMBMode  eMBCurrentMode;
    UCHAR ucMBAddress;
    UCHAR ucPort;
    CHAR szPort[32];
    ULONG ulBaudRate;
    eMBParity eParity;
    UCHAR ucDataBits;
    UCHAR ucStopBit;


     peMBFrameSend peMBFrameSendCur;
     pvMBFrameStart pvMBFrameStartCur;
     pvMBFrameStop pvMBFrameStopCur;
     peMBFrameReceive peMBFrameReceiveCur;
     pvMBFrameClose pvMBFrameCloseCur;

     int      iSerialFd;
 BOOL     bRxEnabled;
 BOOL     bTxEnabled;

 ULONG    ulTimeoutMs;
 UCHAR    ucBuffer[256];//BUF_SIZE
 int      uiRxBufferPos;
 int      uiTxBufferPos;
     struct termios xOldTIO;

ULONG           ulTimeOut;
BOOL            bTimeoutEnable;
 struct timeval xTimeLast;

 UCHAR    ucMBSlaveID[32];//MB_FUNC_OTHER_REP_SLAVEID_BUF
 USHORT   usMBSlaveIDLen;

 int eMBState;

      UCHAR   *ucMBFrame;
     UCHAR    ucRcvAddress;
     UCHAR    ucFunctionCode;
     USHORT   usLength;
     eMBException eException;

 eMBEventType eQueuedEvent;
 BOOL     xEventInQueue;

  volatile eMBSndState eSndState;
 volatile eMBRcvState eRcvState;

 UCHAR  ucRTUBuf[256];//MB_SER_PDU_SIZE_MAX

 volatile UCHAR *pucSndBufferCur;
 volatile USHORT usSndBufferCount;

 volatile USHORT usRcvBufferPos;

/*
 USHORT   usRegInputStart;
 USHORT*   usRegInputBuf;
 USHORT    usRegHoldingStart;
 USHORT*   usRegHoldingBuf;
 USHORT   usRegWriteCmdStart;
 ULONG REG_INPUT_START;
 ULONG REG_INPUT_NREGS;
 ULONG REG_HOLDING_START;
 ULONG REG_HOLDING_NREGS;
*/

BOOL( *pxMBFrameCBByteReceived ) ( fmodbus_t* );
BOOL( *pxMBFrameCBTransmitterEmpty ) ( fmodbus_t* );
BOOL( *pxMBPortCBTimerExpired ) ( fmodbus_t* );

BOOL( *pxMBFrameCBReceiveFSMCur ) ( fmodbus_t* );
BOOL( *pxMBFrameCBTransmitFSMCur ) ( fmodbus_t* );
    pthread_mutex_t xLock;

    fd_set   allset;
    SOCKET xListenSocket;
    SOCKET xClientSocket;

 UCHAR    aucTCPBuf[256 + 7];//MB_TCP_BUF_SIZE
 USHORT   usTCPBufPos;
 USHORT   usTCPFrameBytesLeft;
}fmodbus_t;

#endif /* MBCTX_H */
