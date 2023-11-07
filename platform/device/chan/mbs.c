#include "plt.h"

enum mbs_idx_t
{
    MBSIDX_MDL = 0,
    MBSIDX_EMA = 1,
};

fmodbus_t *MB[4];
/* ----------------------- Defines ------------------------------------------*/
#define REG_HOLDING_START 0x0000
#define REG_HOLDING_NREGS 0x6200
static USHORT usHoldingRegisters[REG_HOLDING_NREGS] = {0};

#define REG_INPUT_START 0x0000
#define REG_INPUT_NREGS 0x5400
static USHORT usInputRegisters[REG_INPUT_NREGS] = {0};

static enum ThreadState {
    STOPPED,
    RUNNING,
    SHUTDOWN
} ePollThreadState;

static pthread_mutex_t xLock = PTHREAD_MUTEX_INITIALIZER;
// static BOOL     bDoExit;

void *pvPollingThread(void *pvParameter)
{
    fmodbus_t *ctx = (fmodbus_t *)pvParameter;
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];

    log_dbg("%s, ++, mbsidx:%d", __func__, ctx->mbsidx);

    if (eMBEnable(ctx) == MB_ENOERR)
    {
        do
        {
            if (eMBPoll(ctx) != MB_ENOERR)
                break;
        } while (TRUE);
    }

    (void)eMBDisable(ctx);

    log_dbg("%s, --, mbsidx:%d", __func__, ctx->mbsidx);

    return NULL;
}

static void on_04_ReadInputRegisters(fmodbus_t *ctx)
{
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];
    // int mbsidx = chan->mbsidx;
    // int mbsdevidx = chan->mbsdevidx;
    // struct cess2000_t *dev = &cess2000[1];
    // short temp = 0;
    // struct bcu_t* bcu = &mdl.catl280a[1].bcu[1];
    // int bmuidx, moduleidx, cellidx, tempidx;

    // if( chan->dbg > 0 ){
    //     log_dbg("%s, chanidx:%d, mbsidx:%d, mbsdevm:%d, mbsdevidx:%d",
    //             __func__, ctx->chanidx, chan->mbsidx, chan->mbsdevm, chan->mbsdevidx);
    // }

    usInputRegisters[0 - REG_INPUT_START] = MDL.ua;
    usInputRegisters[1 - REG_INPUT_START] = MDL.ub;
}

// process 03
static void on_03_ReadMultipleHoldingRegisters(fmodbus_t *ctx)
{
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];
    // int mbsidx = chan->mbsidx;
    // int mbsdevidx = chan->mbsdevidx;
    // struct cess2000_t *dev = &cess2000[1];
    // short temp = 0;
    // struct bcu_t* bcu = &mdl.catl280a[1].bcu[1];
    // int bmuidx, moduleidx, cellidx, tempidx;

    // if( chan->dbg > 0 ){
    //     log_dbg("%s, chanidx:%d, mbsidx:%d, mbsdevm:%d, mbsdevidx:%d",
    //             __func__, ctx->chanidx, chan->mbsidx, chan->mbsdevm, chan->mbsdevidx);
    // }

    usHoldingRegisters[0x6020 - REG_HOLDING_START] = MDL.uab * 10;
    usHoldingRegisters[0x6021 - REG_HOLDING_START] = MDL.ubc * 10;
    usHoldingRegisters[0x6022 - REG_HOLDING_START] = MDL.uca * 10;

    usHoldingRegisters[0x6026 - REG_HOLDING_START] = MDL.ia * 10;
    usHoldingRegisters[0x6027 - REG_HOLDING_START] = MDL.ib * 10;
    usHoldingRegisters[0x6028 - REG_HOLDING_START] = MDL.ic * 10;
}

// 06
static void on_06_WriteSingleHoldingRegister(fmodbus_t *ctx)
{
    MDL.uab = usHoldingRegisters[0x6020 - REG_HOLDING_START] / 10.0;
    MDL.ubc = usHoldingRegisters[0x6021 - REG_HOLDING_START] / 10.0;
    MDL.uca = usHoldingRegisters[0x6022 - REG_HOLDING_START] / 10.0;

    MDL.ia = usHoldingRegisters[0x6026 - REG_HOLDING_START] / 10.0;
    MDL.ib = usHoldingRegisters[0x6027 - REG_HOLDING_START] / 10.0;
    MDL.ic = usHoldingRegisters[0x6028 - REG_HOLDING_START] / 10.0;        
}

eMBErrorCode eMBRegInputCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if (ctx->mbsidx == MBSIDX_MDL)
    {
        if ((usAddress >= REG_INPUT_START) && (usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS))
        {
            // iRegIndex = (int)(usAddress - usRegInputStart_PCS[chan->mbsidx]);
            on_04_ReadInputRegisters(ctx);
            iRegIndex = (int)(usAddress - REG_INPUT_START);
            while (usNRegs > 0)
            {
                *pucRegBuffer++ = (UCHAR)(usInputRegisters[iRegIndex] >> 8);
                *pucRegBuffer++ = (UCHAR)(usInputRegisters[iRegIndex] & 0xFF);
                iRegIndex++;
                usNRegs--;
            }
        }
        else
        {
            eStatus = MB_ENOREG;
        }
        return eStatus;
    }

    return MB_ENOREG;
}

eMBErrorCode eMBRegHoldingCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;
    int i = 0;

    if (ctx->mbsidx == MBSIDX_MDL)
    { // ctn modbus tcp slave
        if ((usAddress >= REG_HOLDING_START) && (usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS))
        {
            iRegIndex = (int)(usAddress - REG_HOLDING_START);
            switch (eMode)
            {
            /* Pass current register values to the protocol stack. */
            case MB_REG_READ:
                on_03_ReadMultipleHoldingRegisters(ctx);
                while (usNRegs > 0)
                {
                    *pucRegBuffer++ = (UCHAR)(usHoldingRegisters[iRegIndex] >> 8);
                    *pucRegBuffer++ = (UCHAR)(usHoldingRegisters[iRegIndex] & 0xFF);
                    iRegIndex++;
                    usNRegs--;
                }
                break;

                /* Update current register values with new values from the
                 * protocol stack. */
            case MB_REG_WRITE:
                while (usNRegs > 0)
                {
                    usHoldingRegisters[iRegIndex] = *pucRegBuffer++ << 8;
                    usHoldingRegisters[iRegIndex] |= *pucRegBuffer++;
                    iRegIndex++;
                    usNRegs--;
                }
                on_06_WriteSingleHoldingRegister(ctx);
                break;
            }
        }
        else
        {
            eStatus = MB_ENOREG;
        }

        return eStatus;
    }
}

eMBErrorCode eMBRegCoilsCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode)
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNDiscrete)
{
    return MB_ENOREG;
}

int mbs_start_MDL()
{
    int ret = 0;
    const UCHAR ucSlaveID[] = {0xAA, 0xBB, 0xCC};
    pthread_t xthrd;

    if (eMBInit(&MB[MBSIDX_MDL], MB_RTU, MDL.adr, MDL.szSerial, 9600, MB_PAR_NONE) != MB_ENOERR)
    {
        log_dbg("%s, eMBTCPInit fail", __func__);
        ret = -1;
    }
    else if (eMBSetSlaveID(MB[MBSIDX_MDL], 0x34, TRUE, ucSlaveID, 3) != MB_ENOERR)
    {
        log_dbg("%s, eMBSetSlaveID fail", __func__);
        ret = -2;
    }
    else
    {
        MB[MBSIDX_MDL]->mbsidx = MBSIDX_MDL;
        if (pthread_create(&xthrd, NULL, pvPollingThread, MB[MBSIDX_MDL]) != 0)
        {
            log_dbg("%s, pthread_create fail", __func__);
            ret = -3;
        }
        else
        {
            log_dbg("%s, start ok", __func__);
        }
    }

    return ret;
}