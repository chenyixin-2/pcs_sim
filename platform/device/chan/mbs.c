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

static void onReadInputBuf_CESS2000(fmodbus_t *ctx)
{
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];
    // int mbsidx = chan->mbsidx;
    // int mbsdevidx = chan->mbsdevidx;
    //struct cess2000_t *dev = &cess2000[1];
    //short temp = 0;
    // struct bcu_t* bcu = &mdl.catl280a[1].bcu[1];
    // int bmuidx, moduleidx, cellidx, tempidx;

    // if( chan->dbg > 0 ){
    //     log_dbg("%s, chanidx:%d, mbsidx:%d, mbsdevm:%d, mbsdevidx:%d",
    //             __func__, ctx->chanidx, chan->mbsidx, chan->mbsdevm, chan->mbsdevidx);
    // }

    //usRegInputReg_ZH200[0 - REG_INPUT_START_ZH200] = ctn_get_cmd();
    //usRegInputReg_ZH200[1 - REG_INPUT_START_ZH200] = 0;
}

// process 03
static void onReadRegHoldingBuf_ENJOY100KW(fmodbus_t *ctx)
{

    // struct chan_t* chan = &mdl.chan[ctx->chanidx];
    // int mbsidx = chan->mbsidx;
    // int mbsdevidx = chan->mbsdevidx;
    //struct cess2000_t *dev = &cess2000[1];
    //short temp = 0;
    // struct bcu_t* bcu = &mdl.catl280a[1].bcu[1];
    // int bmuidx, moduleidx, cellidx, tempidx;

    // if( chan->dbg > 0 ){
    //     log_dbg("%s, chanidx:%d, mbsidx:%d, mbsdevm:%d, mbsdevidx:%d",
    //             __func__, ctx->chanidx, chan->mbsidx, chan->mbsdevm, chan->mbsdevidx);
    // }

    usRegHoldingBuf_ENJOY100KW[0x6020 - REG_HOLDING_START_ENJOY100KW] = mdl.uab*10;
    usRegHoldingBuf_ENJOY100KW[0x6021 - REG_HOLDING_START_ENJOY100KW] = mdl.ubc*10;
    usRegHoldingBuf_ENJOY100KW[0x6022 - REG_HOLDING_START_ENJOY100KW] = mdl.uca*10;

    usRegHoldingBuf_ENJOY100KW[0x6026 - REG_HOLDING_START_ENJOY100KW] = mdl.ia*10;
    usRegHoldingBuf_ENJOY100KW[0x6027 - REG_HOLDING_START_ENJOY100KW] = mdl.ib*10;
    usRegHoldingBuf_ENJOY100KW[0x6028 - REG_HOLDING_START_ENJOY100KW] = mdl.ic*10;

#if 0
    usRegHoldingBuf_ENJOY100KW[3 - REG_HOLDING_START_ENJOY100KW] = 0;
    usRegHoldingBuf_ENJOY100KW[4 - REG_HOLDING_START_ENJOY100KW] = 0;
    // usRegHoldingBuf_ENJOY100KW[5 - REG_HOLDING_START_ENJOY100KW] = ;   // reserved
    // usRegHoldingBuf_ENJOY100KW[6 - REG_HOLDING_START_ENJOY100KW] = ;   // reserved
    usRegHoldingBuf_ENJOY100KW[7 - REG_HOLDING_START_ENJOY100KW] = ctn_get_tick();
    // usRegHoldingBuf_ENJOY100KW[8 - REG_HOLDING_START_ENJOY100KW] = ctn_get_tick(); // 本级心跳，先不管
    // usRegHoldingBuf_ENJOY100KW[9 - REG_HOLDING_START_ENJOY100KW] = ; // reserved

    usRegHoldingBuf_ENJOY100KW[10 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_state();
    usRegHoldingBuf_ENJOY100KW[11 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_err();
    usRegHoldingBuf_ENJOY100KW[12 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_ap();  // 当前有功功率
    usRegHoldingBuf_ENJOY100KW[13 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_aps(); // 有功功率设定值

    usRegHoldingBuf_ENJOY100KW[14 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_chgable();
    usRegHoldingBuf_ENJOY100KW[15 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_dhgable();

    usRegHoldingBuf_ENJOY100KW[16 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_soc(); // soc
    usRegHoldingBuf_ENJOY100KW[17 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_soh(); // soh
    usRegHoldingBuf_ENJOY100KW[18 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_cellvmax() * 1000));
    usRegHoldingBuf_ENJOY100KW[19 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_cellvave() * 1000));
    usRegHoldingBuf_ENJOY100KW[20 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_cellvmin() * 1000));
    usRegHoldingBuf_ENJOY100KW[21 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_celltmax()));
    usRegHoldingBuf_ENJOY100KW[22 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_celltave()));
    usRegHoldingBuf_ENJOY100KW[23 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)((int)(pack_get_celltmin()));

    usRegHoldingBuf_ENJOY100KW[125 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_norm_cap(); // 额定容量
    usRegHoldingBuf_ENJOY100KW[126 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)ctn_get_norm_pow(); // 额定功率

    usRegHoldingBuf_ENJOY100KW[138 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)mdl.version[0]; // 硬件版本号
    usRegHoldingBuf_ENJOY100KW[139 - REG_HOLDING_START_ENJOY100KW] = (unsigned short)mdl.version[1]; // 软件版本号
    #endif
}

// 06
static void onWriteRegHoldingBuf_CESS2000(fmodbus_t *ctx)
{
#if 0
    // log_dbg("%s, ++ register write", __func__);
    if (usRegHoldingBuf_ENJOY100KW[0 - REG_HOLDING_START_ENJOY100KW] != 0)
    {
        log_info("%s,get cmd(%d) form host", __func__, usRegHoldingBuf_ENJOY100KW[0 - REG_HOLDING_START_ENJOY100KW]);
        ctn_set_cmd(usRegHoldingBuf_ENJOY100KW[0 - REG_HOLDING_START_ENJOY100KW]);
        usRegHoldingBuf_ENJOY100KW[0 - REG_HOLDING_START_ENJOY100KW] = 0;
    }

    if ((short)usRegHoldingBuf_ENJOY100KW[13 - REG_HOLDING_START_ENJOY100KW] != ctn_get_aps())
    {
        log_info("%s,get new aps(%d) form host", __func__, (short)usRegHoldingBuf_ENJOY100KW[13 - REG_HOLDING_START_ENJOY100KW]);
        ctn_set_aps((short)usRegHoldingBuf_ENJOY100KW[13 - REG_HOLDING_START_ENJOY100KW]);
    }
    // else
    // {
    //     log_dbg("%s, aps not change, set value : %d, ctn value : %d", __func__, usRegHoldingBuf_ENJOY100KW[13 - REG_HOLDING_START_ENJOY100KW], ctn_get_aps());
    // }
#endif
    // log_dbg("%s, --", __func__);
}

static void prepareRegHoldingBuf_EMA(fmodbus_t *ctx)
{
#if 0
    float temp_f = 0;
    unsigned short temp_u16 = 0;
    signed short temp_s16 = 0;
    signed int temp_s32 = 0;
    unsigned short *ptemp = NULL;

    // log_info("%s,running!!!!!",__func__);

    if (ems_get_mode() == EMSMOD_NONE)
    {
        usRegHoldingBuf_EMA[1000 - REG_HOLDING_START_EMA] = 0;
        usRegHoldingBuf_EMA[1001 - REG_HOLDING_START_EMA] = 0;
    }
    else if (ems_get_mode() == EMSMOD_PCURV)
    {
        usRegHoldingBuf_EMA[1000 - REG_HOLDING_START_EMA] = 0;
        usRegHoldingBuf_EMA[1001 - REG_HOLDING_START_EMA] = 1;
    }

    temp_f = 600.0;

    usRegHoldingBuf_EMA[1002 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1003 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    usRegHoldingBuf_EMA[1004 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1005 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    temp_f = ctn_get_soc();
    usRegHoldingBuf_EMA[1006 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1007 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    // cell fault state-------reserve

    temp_f = 2000.0;
    usRegHoldingBuf_EMA[1010 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1011 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));
    temp_f = 1800;
    usRegHoldingBuf_EMA[1012 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1013 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    temp_f = pcs_get_chg_e_total();
    usRegHoldingBuf_EMA[1014 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1015 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    temp_f = pcs_get_dhg_e_total();
    usRegHoldingBuf_EMA[1016 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1017 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    usRegHoldingBuf_EMA[1018 - REG_HOLDING_START_EMA] = 0;
    usRegHoldingBuf_EMA[1019 - REG_HOLDING_START_EMA] = ctn_get_tick();

    temp_s32 = ctn_cal_get_cycle();
    usRegHoldingBuf_EMA[1020 - REG_HOLDING_START_EMA] = *(unsigned int *)&temp_s32 >> 16;
    usRegHoldingBuf_EMA[1021 - REG_HOLDING_START_EMA] = *(unsigned int *)&temp_s32;

    usRegHoldingBuf_EMA[1022 - REG_HOLDING_START_EMA] = 0;
    usRegHoldingBuf_EMA[1023 - REG_HOLDING_START_EMA] = ctn_get_state();

    temp_s32 = ctn_get_ap();
    usRegHoldingBuf_EMA[1024 - REG_HOLDING_START_EMA] = *(unsigned int *)&temp_s32 >> 16;
    usRegHoldingBuf_EMA[1025 - REG_HOLDING_START_EMA] = *(unsigned int *)&temp_s32;

    temp_f = pcs_get_grid_freq();
    usRegHoldingBuf_EMA[1026 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f)) >> 16;
    usRegHoldingBuf_EMA[1027 - REG_HOLDING_START_EMA] = *((unsigned int *)(&temp_f));

    usRegHoldingBuf_EMA[1028 - REG_HOLDING_START_EMA] = 0;
    usRegHoldingBuf_EMA[1029 - REG_HOLDING_START_EMA] = ctn_get_err();
    #endif
}

static void procRegHoldingBuf_EMA(fmodbus_t *ctx)
{
#if 0
    unsigned int temp = 0;
    int aps = 0;

    temp = usRegHoldingBuf_EMA[2000 - REG_HOLDING_START_EMA];
    // log_info("%s,temp(%d)",__func__,temp);
    temp = (temp << 16) + usRegHoldingBuf_EMA[2001 - REG_HOLDING_START_EMA];
    // log_info("%s,temp(%d)",__func__,temp);
    aps = *((float *)(&temp));
    // if(aps != 0){
    log_info("%s,get aps(%d),tepm(%d),2000(%d),2001(%d)form host", __func__, aps, temp, usRegHoldingBuf_EMA[2000 - REG_HOLDING_START_EMA], usRegHoldingBuf_EMA[2001 - REG_HOLDING_START_EMA]);
    if (ems_get_mode() == EMSMOD_NONE)
        ctn_set_aps(aps);
    //}
    #endif
}

eMBErrorCode eMBRegInputCB(fmodbus_t *ctx, UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs)
{
    eMBErrorCode eStatus = MB_ENOERR;
    int iRegIndex;
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];
    // if (chan->mbsidx >= 1 && chan->mbsidx <= PCS_NBR_MAX)
    if (ctx->mbsidx == MBSIDX_ENJOY100KW)
    {
        // if ((usAddress >= REG_INPUT_START_PCS) && (usAddress + usNRegs <= REG_INPUT_START_PCS + REG_INPUT_NREGS_PCS))
        // if ((usAddress >= REG_HOLDING_START_ENJOY100KW) && (usAddress + usNRegs <= REG_HOLDING_START_ENJOY100KW + REG_HOLDING_NREGS_ENJOY100KW))
        if ((usAddress >= REG_INPUT_START_ZH200) && (usAddress + usNRegs <= REG_INPUT_START_ZH200 + REG_INPUT_REG_ZH200))
        {
            // iRegIndex = (int)(usAddress - usRegInputStart_PCS[chan->mbsidx]);
            onReadInputBuf_CESS2000(ctx);
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
    // struct chan_t* chan = &mdl.chan[ctx->chanidx];

    // if( chan->dbg > 0 ){
    //     log_dbg("%s, chanidx:%d, mbsidx:%d, mbsdevm:%d, mbsdevidx:%d, usAddress:%d, usNRegs:%d, eMode:%d",
    //                                                             __func__, ctx->chanidx, chan->mbsidx, chan->mbsdevm, chan->mbsdevidx,
    //                                                             usAddress, usNRegs, eMode);
    // }
    // if( chan->en == 0 ){
    //     return MB_EIO;
    // }
    if (ctx->mbsidx == MBSIDX_ENJOY100KW){ // ctn modbus tcp slave
        if ((usAddress >= REG_HOLDING_START_ENJOY100KW) && (usAddress + usNRegs <= REG_HOLDING_START_ENJOY100KW + REG_HOLDING_NREGS_ENJOY100KW))
        {
            iRegIndex = (int)(usAddress - REG_HOLDING_START_ENJOY100KW);
            switch (eMode)
            {
            /* Pass current register values to the protocol stack. */
            case MB_REG_READ:
                onReadRegHoldingBuf_ENJOY100KW(ctx);
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
                onWriteRegHoldingBuf_CESS2000(ctx);
                break;
            }
        }
        else
        {
            eStatus = MB_ENOREG;
        }

        return eStatus;
    }
    else if (ctx->mbsidx == MBSIDX_EMA)
    { // ctn modbus tcp slave
        if ((usAddress >= REG_HOLDING_START_EMA) && (usAddress + usNRegs <= REG_HOLDING_START_EMA + REG_HOLDING_NREGS_EMA))
        {
            iRegIndex = (int)(usAddress - REG_HOLDING_START_EMA);
            // log_info("%s,%d",__func__,iRegIndex);
            switch (eMode)
            {
            /* Pass current register values to the protocol stack. */
            case MB_REG_READ:
                prepareRegHoldingBuf_EMA(ctx);
                while (usNRegs > 0)
                {
                    *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf_EMA[iRegIndex] >> 8);
                    *pucRegBuffer++ = (UCHAR)(usRegHoldingBuf_EMA[iRegIndex] & 0xFF);
                    iRegIndex++;
                    usNRegs--;
                }
                break;

                /* Update current register values with new values from the
                 * protocol stack. */
            case MB_REG_WRITE:
                while (usNRegs > 0)
                {
                    usRegHoldingBuf_EMA[iRegIndex] = *pucRegBuffer++ << 8;
                    usRegHoldingBuf_EMA[iRegIndex] |= *pucRegBuffer++;
                    iRegIndex++;
                    usNRegs--;
                }
                procRegHoldingBuf_EMA(ctx);
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

int mbs_start_Enjoy100kW(int port)
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

int mbs_start_EMA(int port)
{
    int ret = 0;
    const UCHAR ucSlaveID[] = {0xAA, 0xBB, 0xCC};
    pthread_t xthrd;

    if (eMBTCPInit(&MB[MBSIDX_EMA], port) != MB_ENOERR)
    {
        log_dbg("%s, eMBTCPInit fail", __func__);
        ret = -1;
    }
    else if (eMBSetSlaveID(MB[MBSIDX_EMA], 0x34, TRUE, ucSlaveID, 3) != MB_ENOERR)
    {
        log_dbg("%s, eMBSetSlaveID fail", __func__);
        ret = -2;
    }
    else
    {
        MB[MBSIDX_EMA]->mbsidx = MBSIDX_EMA;
        if (pthread_create(&xthrd, NULL, pvPollingThread, MB[MBSIDX_EMA]) != 0)
        {
            log_dbg("%s, pthread_create fail", __func__);
            ret = -3;
        }
        else
        {
            log_dbg("%s, port:%d,start ok", __func__, port);
        }
    }

    return ret;
}