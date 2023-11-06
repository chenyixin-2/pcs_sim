#include "mbs.h"
#include "log.h"
#include "mb.h"
#include "mbctx.h"
#include "plt.h"
#include "port.h"

enum mbs_idx_t
{
    MBSIDX_ENJOY100KW = 0,
    MBSIDX_EMA = 1,
};

fmodbus_t *MB[4];
/* ----------------------- Defines ------------------------------------------*/
#define REG_HOLDING_START_ENJOY100KW 0x0000
#define REG_HOLDING_NREGS_ENJOY100KW 0x6200
// static USHORT   usRegHoldingStart_CTN[CTN_NBR_MAX + 1];
static USHORT usRegHoldingBuf_ENJOY100KW[REG_HOLDING_NREGS_ENJOY100KW] = {0};

#define REG_INPUT_START_ZH200 0x0000
#define REG_INPUT_REG_ZH200 0x5400
static USHORT usRegInputReg_ZH200[REG_INPUT_REG_ZH200] = {0};

#define REG_HOLDING_START_EMA 0x0000
#define REG_HOLDING_NREGS_EMA 0x5400
// static USHORT   usRegHoldingStart_CTN[CTN_NBR_MAX + 1];
static USHORT usRegHoldingBuf_EMA[REG_HOLDING_NREGS_EMA] = {0};

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

static void on_04_ReadInputBuf_CESS2000(fmodbus_t *ctx)
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

    // usRegInputReg_ZH200[0 - REG_INPUT_START_ZH200] = ctn_get_cmd();
    // usRegInputReg_ZH200[1 - REG_INPUT_START_ZH200] = 0;
}

// process 03
static void on_03_ReadRegHoldingBuf_ENJOY100KW(fmodbus_t *ctx)
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

    usRegHoldingBuf_ENJOY100KW[0x6020 - REG_HOLDING_START_ENJOY100KW] = mdl.uab * 10;
    usRegHoldingBuf_ENJOY100KW[0x6021 - REG_HOLDING_START_ENJOY100KW] = mdl.ubc * 10;
    usRegHoldingBuf_ENJOY100KW[0x6022 - REG_HOLDING_START_ENJOY100KW] = mdl.uca * 10;

    usRegHoldingBuf_ENJOY100KW[0x6026 - REG_HOLDING_START_ENJOY100KW] = mdl.ia * 10;
    usRegHoldingBuf_ENJOY100KW[0x6027 - REG_HOLDING_START_ENJOY100KW] = mdl.ib * 10;
    usRegHoldingBuf_ENJOY100KW[0x6028 - REG_HOLDING_START_ENJOY100KW] = mdl.ic * 10;
}

// 06
static void on_06_WriteRegHoldingBuf_CESS2000(fmodbus_t *ctx)
{
    mdl.uab = usRegHoldingBuf_ENJOY100KW[0x6020 - REG_HOLDING_START_ENJOY100KW] / 10.0;
    mdl.ubc = usRegHoldingBuf_ENJOY100KW[0x6021 - REG_HOLDING_START_ENJOY100KW] / 10.0;
    mdl.uca = usRegHoldingBuf_ENJOY100KW[0x6022 - REG_HOLDING_START_ENJOY100KW] / 10.0;

    mdl.ia = usRegHoldingBuf_ENJOY100KW[0x6026 - REG_HOLDING_START_ENJOY100KW] / 10.0;
    mdl.ib = usRegHoldingBuf_ENJOY100KW[0x6027 - REG_HOLDING_START_ENJOY100KW] / 10.0;
    mdl.ic = usRegHoldingBuf_ENJOY100KW[0x6028 - REG_HOLDING_START_ENJOY100KW] / 10.0;
        
}

eMBErrorCode eMBRegInputCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;

    if (ctx->mbsidx == MBSIDX_ENJOY100KW)
    {
        if ((usAddress >= REG_INPUT_START_ZH200) && (usAddress + usNRegs <= REG_INPUT_START_ZH200 + REG_INPUT_REG_ZH200))
        {
            // iRegIndex = (int)(usAddress - usRegInputStart_PCS[chan->mbsidx]);
            on_04_ReadInputBuf_CESS2000(ctx);
            iRegIndex = (int)(usAddress - REG_INPUT_START_ZH200);
            while (usNRegs > 0)
            {
                *pucRegBuffer++ = (UCHAR)(usRegInputReg_ZH200[iRegIndex] >> 8);
                *pucRegBuffer++ = (UCHAR)(usRegInputReg_ZH200[iRegIndex] & 0xFF);
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

    if (ctx->mbsidx == MBSIDX_ENJOY100KW)
    { // ctn modbus tcp slave
        if ((usAddress >= REG_HOLDING_START_ENJOY100KW) && (usAddress + usNRegs <= REG_HOLDING_START_ENJOY100KW + REG_HOLDING_NREGS_ENJOY100KW))
        {
            iRegIndex = (int)(usAddress - REG_HOLDING_START_ENJOY100KW);
            switch (eMode)
            {
            /* Pass current register values to the protocol stack. */
            case MB_REG_READ:
                on_03_ReadRegHoldingBuf_ENJOY100KW(ctx);
                while (usNRegs > 0)
                {
                    *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf_ENJOY100KW[iRegIndex] >> 8);
                    *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf_ENJOY100KW[iRegIndex] & 0xFF);
                    iRegIndex++;
                    usNRegs--;
                }
                break;

                /* Update current register values with new values from the
                 * protocol stack. */
            case MB_REG_WRITE:
                while (usNRegs > 0)
                {
                    usRegHoldingBuf_ENJOY100KW[iRegIndex] = *pucRegBuffer++ << 8;
                    usRegHoldingBuf_ENJOY100KW[iRegIndex] |= *pucRegBuffer++;
                    iRegIndex++;
                    usNRegs--;
                }
                on_06_WriteRegHoldingBuf_CESS2000(ctx);
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

int mbs_start_Enjoy100kW()
{
    int ret = 0;
    const UCHAR ucSlaveID[] = {0xAA, 0xBB, 0xCC};
    pthread_t xthrd;

    if (eMBInit(&MB[MBSIDX_ENJOY100KW], MB_RTU, mdl.adr, mdl.szser, 9600, MB_PAR_NONE) != MB_ENOERR)
    {
        log_dbg("%s, eMBTCPInit fail", __func__);
        ret = -1;
    }
    else if (eMBSetSlaveID(MB[MBSIDX_ENJOY100KW], 0x34, TRUE, ucSlaveID, 3) != MB_ENOERR)
    {
        log_dbg("%s, eMBSetSlaveID fail", __func__);
        ret = -2;
    }
    else
    {
        MB[MBSIDX_ENJOY100KW]->mbsidx = MBSIDX_ENJOY100KW;
        if (pthread_create(&xthrd, NULL, pvPollingThread, MB[MBSIDX_ENJOY100KW]) != 0)
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