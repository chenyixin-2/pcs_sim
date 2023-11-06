#include "sta.h"
#include "plt.h"

static int sta_dbcb_0(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    int i;
    struct dbcbparam_t *pcbparam = (struct dbcbparam_t *)para;
    struct mdl_t *sta = &mdl;

    pcbparam->nrow++;
    log_dbg("%s, ++, row:%d; col:%d", __func__, pcbparam->nrow, ncolumn);

    for (i = 0; i < ncolumn; i++)
    {
        if (strcmp("info", columnname[i]) == 0)
        {
            strcpy(sta->szinfo, columnvalue[i]);
        }
        else if (strcmp("projid", columnname[i]) == 0)
        {
            strcpy(sta->szprojId, columnvalue[i]);
        }
        else if (strcmp("time_zone", columnname[i]) == 0)
        {
            sta->time_zone = atoi(columnvalue[i]);
        }
        else if (strcmp("idx", columnname[i]) == 0)
        {
            sta->idx = atoi(columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, --,ret:%d", __func__, pcbparam->ret);
    return 0;
}

int sta_get_timezone()
{
    return mdl.time_zone;
}

char *sta_get_prjid(void)
{
    return mdl.szprojId;
}

int sta_get_norm_cap()
{
    return mdl.pow.norm_cap;
}

int sta_get_norm_pow()
{
    return mdl.pow.norm_pow;
}

char *sta_get_err_str()
{
    return mdl.sm.szerrcode;
}

int sta_get_state(void)
{
    return mdl.sm.state;
}

void sta_set_dhgable(void)
{
    mdl.pow.bdhgable = 1;
}
void sta_reset_dhgable(void)
{
    mdl.pow.bdhgable = 0;
}

int sta_get_dhgable()
{
    return mdl.pow.bdhgable;
}

int sta_get_chgable()
{
    return mdl.pow.bchgable;
}

void sta_set_chgable(void)
{
    mdl.pow.bchgable = 1;
}
void sta_reset_chgable(void)
{
    mdl.pow.bchgable = 0;
}

int sta_set_aps(int val)
{
    int ret = 0;
    mdl.pow.active_p_set = val;

    // log_dbg( "%s, val:%d, ret:%d", __func__, val, ret);
    return ret;
}

int sta_get_ap()
{
    return mdl.pow.active_p;
}

double sta_get_soc()
{
    return mdl.pow.soc;
}

int sta_get_aps()
{
    return mdl.pow.active_p_set;
}

char *sta_get_state_str()
{
    return mdl.sm.szstate;
}

void sta_set_state(int state, int errcode)
{
    struct mdl_t *dev = &mdl;
    mdl.sm.step = 0;
    mdl.sm.state = state;
    mdl.sm.err = errcode;

    switch (state)
    {
    case SMST_LAUNCH:
        strcpy(dev->sm.szstate, "launch");
        break;

    case SMST_STDBY:
        strcpy(dev->sm.szstate, "stdby");
        break;

    case SMST_STOP:
        strcpy(dev->sm.szstate, "stop");
        break;

    case SMST_READY:
        strcpy(dev->sm.szstate, "ready");
        break;

    case SMST_DHG:
        strcpy(dev->sm.szstate, "dhg");
        break;

    case SMST_CHG:
        strcpy(dev->sm.szstate, "chg");
        break;

    case SMST_ERR:
        strcpy(dev->sm.szstate, "err");
        break;

    default:
        strcpy(dev->sm.szstate, "unknown");
        break;
    }

    switch (errcode)
    {
    case STAERR_NONE:
        strcpy(dev->sm.szerrcode, "none");
        break;

    /* launch */
    case STAERR_LAUNCH_WAIT_ALL_CTN_STDBY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "launch wait all ctn stdby timeout");
        break;

    case STAERR_LAUNCH_WAIT_ALL_METER_READY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "launch wait all meter ready timeout");
        break;

    /* stdby */
    case STAERR_STDBY_OFFLINE_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "stdby ctn offline detected");
        break;

    case STAERR_STDBY_NOT_READY_METER_DETECTED:
        strcpy(dev->sm.szerrcode, "stdby not ready meter detected");
        break;

    case STAERR_STDBY_WAIT_ALL_CTN_STOP_TIMEOUT:
        strcpy(dev->sm.szerrcode, "stdby wait all ctn stop timeout");
        break;

    /* stop */
    case STAERR_STOP_OFFLINE_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "stop offline ctn detected");
        break;

    case STAERR_STOP_NOT_READY_METER_DETECTED:
        strcpy(dev->sm.szerrcode, "stop not ready meter detected");
        break;

    case STAERR_STOP_WAIT_ALL_CTN_STDBY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "stop wait all ctn stdby timeout");
        break;

    case STAERR_STOP_WAIT_ALL_CTN_READY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "stop wait all ctn ready timeout");
        break;

    /* ready */
    case STAERR_READY_NOT_READY_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "ready not ready ctn detected");
        break;

    case STAERR_READY_NOT_READY_METER_DETECTED:
        strcpy(dev->sm.szerrcode, "ready not ready meter detected");
        break;

    case STAERR_READY_DISPOW_FAIL:
        strcpy(dev->sm.szerrcode, "ready dis pow fail");
        break;

    case STAERR_READY_WAIT_DHG_TIMEOUT:
        strcpy(dev->sm.szerrcode, "ready wait dhg timeout");
        break;

    case STAERR_READY_WAIT_CHG_TIMEOUT:
        strcpy(dev->sm.szerrcode, "ready wait chg timeout");
        break;

    case STAERR_READY_WAIT_ALL_CTN_STOP_TIMEOUT:
        strcpy(dev->sm.szerrcode, "ready wait all ctn stop timeout");
        break;

    /* dhg */
    case STAERR_DHG_DISPOW_FAIL:
        strcpy(dev->sm.szerrcode, "dhg dis pow fail");
        break;

    case STAERR_DHG_WAIT_ALL_CTN_READY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "dhg wait all ctn ready timeout");
        break;

    case STAERR_DHG_NOT_READY_METER_DETECTED:
        strcpy(dev->sm.szerrcode, "dhg not ready meter dectected");
        break;

    case STAERR_DHG_ERR_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "dhg err ctn dectected");
        break;

    case STAERR_DHG_OFFLINE_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "dhg offline ctn dectected");
        break;

    /* chg */
    case STAERR_CHG_DISPOW_FAIL:
        strcpy(dev->sm.szerrcode, "chg dis pow fail");
        break;

    case STAERR_CHG_WAIT_ALL_CTN_READY_TIMEOUT:
        strcpy(dev->sm.szerrcode, "chg wait all ctn ready timeout");
        break;

    case STAERR_CHG_NOT_READY_METER_DETECTED:
        strcpy(dev->sm.szerrcode, "chg not ready meter dectected");
        break;

    case STAERR_CHG_ERR_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "chg err ctn dectected");
        break;

    case STAERR_CHG_OFFLINE_CTN_DETECTED:
        strcpy(dev->sm.szerrcode, "chg offline ctn dectected");
        break;

    default:
        strcpy(dev->sm.szerrcode, "unknown");
        break;
    }

    log_dbg("%s, state:%d, errcode:%d", __func__, state, errcode);
}

int sta_send_cmd(int cmd)
{
    int ret = 0;

    mdl.cmd = cmd;

    log_dbg("%s, cmd:%d, ret:%d", __func__, cmd, ret);
    return ret;
}

static void sta_sm_launch(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;

    if (*step == 0)
    { // reserved
        // log_dbg("%s, state:%s, step:%d, all ctn online cmd sent", __func__, szstate,*step);
        // ctn_send_cmd_all(CMD_SM_ONLINE);
        log_dbg("%s, state:%s, step:%d, wait stdby cmd to continue", __func__, szstate, *step);
        *step = 10;
        *count = 0;
    }
    else if (*step == 10)
    { // wait stdby cmd
        if ((*count)++ >= 600)
        { // 60s
            log_dbg("%s, state:%s, step:%d, wait stdby cmd to continue", __func__, szstate, *step);
            *count = 0;
        }
        if (*cmd == CMD_SM_STDBY)
        { // stdby cmd
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got stdby cmd, all ctn stdby cmd sent, wait and chk", __func__, szstate, *step);
            *step = 100;
            *count = 0;
        }
    }
    else if (*step == 100)
    { /* wait all ctn stdby */
        // if(true)
        // { /* ok */
        //     log_dbg("%s, state:%s, step:%d, chk all ctn stdby ok, all meter ready cmd sent, wait and chk", __func__, szstate, *step);
        //     *step = 200;
        //     *count = 0;
        // }
        // else
        // {
        //     if ((*count)++ >= 100)
        //     { // 10 seconds
        //         log_dbg("%s, state:%s, step:%d, wait all ctn stdby timeout, goto err", __func__, szstate, *step);
        //         sta_set_state(SMST_ERR, STAERR_LAUNCH_WAIT_ALL_CTN_STDBY_TIMEOUT);
        //     }
        //     else
        //     {
        //         log_dbg("%s, state:%s, step:%d, waiting all ctn stdby, count:%d", __func__, szstate, *step, *count);
        //     }
        // }
    }
    else if (*step == 200)
    { /* wait meter ready */
        // if(true)
        // { /* ok */
        //     log_dbg("%s, state:%s, step:%d, chk all meter ready ok, goto stdby", __func__, szstate, *step);
        //     sta_set_state(SMST_STDBY, STAERR_NONE);
        // }
        // else
        // {
        //     if ((*count)++ >= 100)
        //     {
        //         log_dbg("%s, state:%s, step:%d, wait all meter ready timeout, goto err", __func__, szstate, *step);
        //         sta_set_state(SMST_ERR, STAERR_LAUNCH_WAIT_ALL_METER_READY_TIMEOUT);
        //     }
        //     else
        //     {
        //         log_dbg("%s, state:%s, step:%d, waiting all meter ready, count:%d", __func__, szstate, *step, *count);
        //     }
        // }
    }
}

static void sta_sm_stdby(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;
    int *last_aps = &mdl.pow.last_active_p_set;

    if (*step == 0)
    { // reserved
        *step = 10;
    }
    else if (*step == 10)
    { // wait cmd
        if (*cmd == CMD_SM_STOP)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got stop cmd, send all ctn stop cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_STOP);
            *step = 100;
            *count = 0;
        }
        // else if (ctn_has_stat(SMST_OFFLINE) == 0)
        //  if(false)
        //  {
        //      log_dbg("%s, state:%s, step:%d, ctn offline detected, goto err", __func__, szstate, *step);
        //      sta_set_state(SMST_ERR, STAERR_STDBY_OFFLINE_CTN_DETECTED);
        //  }
        //  //else if (meter_chk_stat_all(SMST_READY) != 0)
        //  else if(false)
        //  {
        //      log_dbg("%s, state:%s, step:%d, meter not ready detected, goto err", __func__, szstate, *step);
        //      sta_set_state(SMST_ERR, STAERR_STDBY_NOT_READY_METER_DETECTED);
        //  }
    }
    else if (*step == 100)    
    { /* wait all ctn stop */
        sta_set_state(SMST_STOP, STAERR_NONE);
        // if (ctn_chk_stat_all(SMST_STOP) == 0)
        // { /* ok */
        //     log_dbg("%s, state:%s, step:%d, chk all ctn stop ok, goto stop", __func__, szstate, *step);
        
        // }
        // else
        // {
        //     if ((*count)++ >= 100)
        //     { // 10 seconds
        //         log_dbg("%s, state:%s, step:%d, wait all ctn stop timeout, goto err", __func__, szstate, *step);
        //         sta_set_state(SMST_ERR, STAERR_STDBY_WAIT_ALL_CTN_STOP_TIMEOUT);
        //     }
        //     else
        //     {
        //         log_dbg("%s, state:%s, step:%d, waiting all ctn stop, count:%d", __func__, szstate, *step, *count);
        //     }
        // }
    }
}

static void sta_sm_stop(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;
    int *last_aps = &mdl.pow.last_active_p_set;

    if (*step == 0)
    { // reserved
        *step = 10;
    }
    else if (*step == 10)
    { // wait cmd
        if (*cmd == CMD_SM_STDBY)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got stdby cmd, send all ctn stdby cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_STDBY);
            *step = 100;
            *count = 0;
        }
        else if (*cmd == CMD_SM_READY)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got ready cmd, send all ctn ready cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_READY);
            *step = 200;
            *count = 0;
        }
        // else if (ctn_has_stat(SMST_OFFLINE) == 0)
        // else if (false)
        // {
        //     log_dbg("%s, state:%s, step:%d, ctn offline detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_STOP_OFFLINE_CTN_DETECTED);
        // }
        // else if (false)
        // {
        //     log_dbg("%s, state:%s, step:%d, meter not ready detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_STOP_NOT_READY_METER_DETECTED);
        // }
    }
    else if (*step == 100)
    { /* wait all ctn stdby */
        // if (ctn_chk_stat_all(SMST_STDBY) == 0)
        if (true)
        { /* ok */
            log_dbg("%s, state:%s, step:%d, chk all ctn stdby ok, goto stdby", __func__, szstate, *step);
            sta_set_state(SMST_STDBY, STAERR_NONE);
        }
        else
        {
            if ((*count)++ >= 100)
            { // 10 seconds
                log_dbg("%s, state:%s, step:%d, wait all ctn stdby timeout, goto err", __func__, szstate, *step);
                sta_set_state(SMST_ERR, STAERR_STOP_WAIT_ALL_CTN_STDBY_TIMEOUT);
            }
            else
            {
                log_dbg("%s, state:%s, step:%d, waiting all ctn stdby, count:%d", __func__, szstate, *step, *count);
            }
        }
    }
    else if (*step == 200)
    { /* wait all ctn ready */
        // if (ctn_chk_stat_all(SMST_READY) == 0)
        if (FALSE)
        { /* ok */
            log_dbg("%s, state:%s, step:%d, chk all ctn ready ok, goto ready", __func__, szstate, *step);
            sta_set_state(SMST_READY, STAERR_NONE);
            //*aps = 0;
            //*last_aps = *aps;
        }
        else
        {
            if ((*count)++ >= 8000)
            { // 10 seconds
                log_dbg("%s, state:%s, step:%d, wait all ctn ready timeout, goto err", __func__, szstate, *step);
                sta_set_state(SMST_ERR, STAERR_STOP_WAIT_ALL_CTN_READY_TIMEOUT);
            }
            else
            {
                log_dbg("%s, state:%s, step:%d, waiting all ctn ready, count:%d", __func__, szstate, *step, *count);
            }
        }
    }
}

static void sta_sm_ready(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;
    int *aps = &mdl.pow.active_p_set;
    int *last_aps = &mdl.pow.last_active_p_set;

    if (*step == 0)
    { // entry
        *aps = 0;
        *last_aps = *aps;
        *step = 10;
        // ctn_disable_bsytik_all();
    }
    else if (*step == 10)
    { /* wait and chk*/
        if (*cmd == CMD_SM_STOP)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got stop cmd, send all ctn stop cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_STOP);
            *step = 100;
            *count = 0;
        }
        else if (*aps != 0)
        { /* !! new aps*/
            // log_dbg("%s, state:%s, step:%d, new aps:%d detected, regulating", __func__, szstate,*step, *aps);
            // ems_sta_aps_regulate();
            if (*aps > 0)
            { /* prepare to dhg */
                if (sta_get_dhgable())
                {
                    // if (ems_sta_aps_dis() == 0)
                    if (false)
                    {
                        *last_aps = *aps;
                        log_dbg("%s, state:%s, step:%d, power dispatch ok, chk to enter dhg", __func__, szstate, *step);
                        *step = 20;
                        *count = 0;
                        // ctn_enable_bsytik_all();
                    }
                    else
                    {
                        log_dbg("%s, state:%s, step:%d, aps dis fail, stay idle", __func__, szstate, *step);
                        *aps = 0;
                    }
                }
            }
            else if (*aps < 0)
            { /* prepare to chg */
                if (sta_get_chgable())
                {
                    // if (ems_sta_aps_dis() == 0)
                    if (false)
                    {
                        log_dbg("%s, state:%s, step:%d, power dispatch ok, chk to enter chg", __func__, szstate, *step);
                        *last_aps = *aps;
                        *step = 30;
                        *count = 0;
                        // ctn_enable_bsytik_all();
                    }
                    else
                    {
                        log_dbg("%s, state:%s, step:%d, aps dis fail, stay idle", __func__, szstate, *step);
                        *aps = 0;
                    }
                }
            }
            else
            { /* stay idle, continue to wait cmd or new aps */
                log_dbg("%s, state:%s, step:%d, aps = 0 after regulate, stay idle", __func__, szstate, *step);
            }
        }
        // else if (ctn_chk_stat_all(SMST_READY) != 0)
    }
    else if (*step == 20)
    { /* chk to enter dhg */
    }
    else if (*step == 30)
    { /* chk to enter chg */
        /// if (ctn_chk_chg_stat() == 0)
        if (true)
        { // idle to chg
            log_dbg("%s, state:%s, step:%d, chk chg ok, goto chg", __func__, szstate, *step);
            sta_set_state(SMST_CHG, STAERR_NONE);
            sta_set_dhgable();
        }
        else
        {
            if ((*count)++ >= 2000) // cyx ：修改为20s
            {                       // 10 seconds
                log_dbg("%s, state:%s, step:%d, wait to chg timeout, goto err", __func__, szstate, *step);
                sta_set_state(SMST_ERR, STAERR_READY_WAIT_CHG_TIMEOUT);
            }
            else
            {
                log_dbg("%s, state:%s, step:%d, waiting to chg, count:%d", __func__, szstate, *step, *count);
            }
        }
    }
    else if (*step == 100)
    { /* chk to stop */
        // if (ctn_chk_stat_all(SMST_STOP) == 0)
        // if (true)
        if (false)
        { // wait ctn stop
            log_dbg("%s, state:%s, step:%d, chk all ctn stop ok, goto stop", __func__, szstate, *step);
            sta_set_state(SMST_STOP, STAERR_NONE);
        }
        else
        {
            if ((*count)++ >= 100)
            { // 10 seconds
                log_dbg("%s, state:%s, step:%d, wait all ctn stop timeout, goto err", __func__, szstate, *step);
                sta_set_state(SMST_ERR, STAERR_READY_WAIT_ALL_CTN_STOP_TIMEOUT);
            }
            else
            {
                log_dbg("%s, state:%s, step:%d, waiting all ctn stop, count:%d", __func__, szstate, *step, *count);
            }
        }
    }
}

static void sta_sm_dhg(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;
    int *aps = &mdl.pow.active_p_set;
    int *last_aps = &mdl.pow.last_active_p_set;

    if (*step == 0)
    { // entry
        *step = 10;
    }
    else if (*step == 10)
    { /* wait and chk */
        if (*cmd == CMD_SM_READY)
        { /* ready cmd */
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got ready cmd, send all ctn ready cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_READY);
            *step = 100;
            *count = 0;
        }
        else if (*aps < 5)
        { /* aps <= 0 0 means idle; <0 means illegal value */
            log_dbg("%s, state:%s, step:%d, aps < 5, send all ctn idle cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_READY);
            *step = 100;
            *count = 0;
        }
        else if (*aps != *last_aps)
        { // new aps(by manual) detected
            // log_dbg("%s, state:%s, step:%d, new aps:%d detected, regulating", __func__, szstate,*step, *aps);
            // ems_sta_aps_regulate();
            if (*aps >= 5)
            {
                log_dbg("%s, state:%s, step:%d, new aps:%d valid, try to dispatch", __func__, szstate, *step, *aps);
                // if (ems_sta_aps_dis() == 0)
                // {
                //     log_dbg("%s, state:%s, step:%d, aps dis ok, stay dhg", __func__, szstate, *step);
                //     *last_aps = *aps;
                // }
                // else
                // {
                //     log_dbg("%s, state:%s, step:%d, aps dis fail, set aps=0", __func__, szstate, *step);
                //     *aps = 0;
                // }
            }
            else
            {
                log_dbg("%s, state:%s, step:%d, new aps:%d invalid, send all ctn idle cmd, wait and chk", __func__, szstate, *step, *aps);
                // ctn_send_cmd_all(CMD_SM_READY);
                *step = 100;
                *count = 0;
            }
        }
        // else if (meter_chk_stat_all(SMST_READY) != 0)
        // {
        //     log_dbg("%s, state:%s, step:%d, not ready meter detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_DHG_NOT_READY_METER_DETECTED);
        // }
        // else if (ctn_has_stat(SMST_ERR) == 0)
        // { /* err ctn */
        //     log_dbg("%s, state:%s, step:%d, err ctn detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_DHG_ERR_CTN_DETECTED);
        // }
        // else if (ctn_has_stat(SMST_OFFLINE) == 0)
        // { /* offline ctn */
        //     log_dbg("%s, state:%s, step:%d, offline ctn detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_DHG_OFFLINE_CTN_DETECTED);
        // }
        // else if (ctn_chk_stat_all(SMST_READY) == 0)
        // {
        //     log_dbg("%s, state:%s, step:%d, all ctn ready detected, goto ready", __func__, szstate, *step);
        //     /* ctn_send_cmd_all(CMD_SM_IDLE); */ /* already all idle, no need to send idle */
        //     sta_reset_dhgable();
        //     sta_set_state(SMST_READY, STAERR_NONE);
        // }
        else
        {
        }
    }
    else if (*step == 100)
    {
        // if (ctn_chk_stat_all(SMST_READY) == 0)
        // { // wait ctn ready
        //     log_dbg("%s, state:%s, step:%d, chk all ctn ready ok, goto ready", __func__, szstate, *step);
        //     sta_set_state(SMST_READY, STAERR_NONE);
        // }
        // else
        // {
        //     if ((*count)++ >= 1000)
        //     { // 10 seconds
        //         log_dbg("%s, state:%s, step:%d, wait all ctn ready timeout, goto err",
        //                 __func__, szstate, *step);
        //         sta_set_state(SMST_ERR, STAERR_DHG_WAIT_ALL_CTN_READY_TIMEOUT);
        //     }
        //     else
        //     {
        //         log_dbg("%s, state:%s, step:%d, waiting all ctn ready, count:%d",
        //                 __func__, szstate, *step, *count);
        //     }
        // }
    }
}

static void sta_sm_chg(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;
    int *aps = &mdl.pow.active_p_set;
    int *last_aps = &mdl.pow.last_active_p_set;

    if (*step == 0)
    { /* entry */
        *step = 10;
    }
    else if (*step == 10)
    { /* wait and chk */
        if (*cmd == CMD_SM_READY)
        { /* ready cmd */
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got ready cmd, send all ctn ready cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_READY);
            *step = 100;
            *count = 0;
        }
        else if (*aps > -5)
        { /* illegal value */
            log_dbg("%s, state:%s, step:%d, aps = 0, send all ctn ready cmd, wait and chk", __func__, szstate, *step);
            // ctn_send_cmd_all(CMD_SM_READY);
            *step = 100;
            *count = 0;
        }
        else if (*aps != *last_aps)
        { // new aps detected
          // log_dbg("%s, state:%s, step:%d, new aps:%d detected, regulating", __func__, szstate,*step, *aps);
          // ems_sta_aps_regulate();
          // if (*aps < -5)
          // { /* continue to chg */
          //     log_dbg("%s, state:%s, step:%d, new aps:%d valid, try to dispatch", __func__, szstate, *step, *aps);
          //     if (ems_sta_aps_dis() == 0)
          //     {
          //         log_dbg("%s, state:%s, step:%d, power dis ok, stay chg", __func__, szstate, *step);
          //         *last_aps = *aps;
          //     }
          //     else
          //     {
          //         log_dbg("%s, state:%s, step:%d, aps dis fail, set aps=0", __func__, szstate, *step);
          //         *aps = 0;
          //     }
          // }
          // else
          // {
          //     log_dbg("%s, state:%s, step:%d, new aps:%d invalid, send all ctn ready cmd, wait and chk", __func__, szstate, *step, *aps);
          //     ctn_send_cmd_all(CMD_SM_READY);
          //     *step = 100;
          //     *count = 0;
          // }
        }
        // else if (meter_chk_stat_all(SMST_READY) != 0)
        // {
        //     log_dbg("%s, state:%s, step:%d, not ready meter detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_CHG_NOT_READY_METER_DETECTED);
        // }
        // else if (ctn_has_stat(SMST_ERR) == 0)
        // { /* some ctn enter err */
        //     log_dbg("%s, state:%s, step:%d, err ctn detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_CHG_ERR_CTN_DETECTED);
        // }
        // else if (ctn_has_stat(SMST_OFFLINE) == 0)
        // { /* some ctn enter offline */
        //     log_dbg("%s, state:%s, step:%d, offline ctn detected, goto err", __func__, szstate, *step);
        //     sta_set_state(SMST_ERR, STAERR_CHG_OFFLINE_CTN_DETECTED);
        // }
        // else if (ctn_chk_stat_all(SMST_READY) == 0)
        // {
        //     log_dbg("%s, state:%s, step:%d, all ctn ready detected, goto ready", __func__, szstate, *step);
        //     /*ctn_send_cmd_all(CMD_SM_IDLE);*/ /* already all idel, no need to send idle cmd */
        //     sta_reset_chgable();
        //     sta_set_state(SMST_READY, STAERR_NONE);
        // }
        else
        {
        }
    }
    else if (*step == 100)
    {
        // if (ctn_chk_stat_all(SMST_READY) == 0)
        // { // wait ctn ready
        //     log_dbg("%s, state:%s, step:%d, chk all ctn ready ok, goto ready", __func__, szstate, *step);
        //     sta_set_state(SMST_READY, STAERR_NONE);
        // }
        // else
        // {
        //     if ((*count)++ >= 1000)
        //     { // 10 seconds
        //         log_dbg("%s, state:%s, step:%d, wait all ctn ready timeout, goto err", __func__, szstate, *step);
        //         sta_set_state(SMST_ERR, STAERR_CHG_WAIT_ALL_CTN_READY_TIMEOUT);
        //     }
        //     else
        //     {
        //         log_dbg("%s, state:%s, step:%d, waiting all ctn ready, count:%d", __func__, szstate, *step, *count);
        //     }
        // }
    }
}

static void sta_sm_err(void)
{
    int *step = &mdl.sm.step;
    int *count = &mdl.sm.count;
    int *cmd = &mdl.cmd;
    char *szstate = mdl.sm.szstate;

    if (*step == 0)
    { /* entry */
        *step = 10;
        // ctn_disable_bsytik_all();
        // ctn_send_cmd_all(CMD_SM_READY);
    }
    else if (*step == 10)
    { // wait cmd
        if (*cmd == CMD_SM_STDBY)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, got stdby cmd, try to enter stdby from pwrup", __func__, szstate, *step);
            sta_set_state(SMST_LAUNCH, STAERR_NONE);
            sta_send_cmd(CMD_SM_STDBY);
        }
    }
}

void sta_sm(void)
{
    struct mdl_t *dev = &mdl;
    int idx;
    double apsum, capsum, powsum, leftcapsum;
    /* state machine timing statistics */
    double tseclipsed = 0.0;
    double ts = 0.0;

    if (dev->sm.timing_timer < 0)
    { /* rest */
        dev->sm.timing_timer = 0;
        ts = misc_gettimeofday();
        dev->sm.timing_tslastrun = ts;
        dev->sm.timing_totalcnt = 0;
        dev->sm.timing_totaltime = 0;
        dev->sm.timing_ave = -1.0;
        dev->sm.timing_max = -1.0;
        dev->sm.timing_cur = -1.0;
    }
    else
    {
        if (dev->sm.timing_timer++ >= 100)
        { /* cal every 100 times */
            dev->sm.timing_timer = 0;
            ts = misc_gettimeofday();
            tseclipsed = ts - dev->sm.timing_tslastrun;
            dev->sm.timing_totalcnt  += 1;
            dev->sm.timing_totaltime += tseclipsed;
            dev->sm.timing_tslastrun = ts;
            dev->sm.timing_ave = dev->sm.timing_totaltime / dev->sm.timing_totalcnt / 100;
            dev->sm.timing_cur = tseclipsed / 100;
            if (dev->sm.timing_cur > dev->sm.timing_max)
            {
                dev->sm.timing_max = dev->sm.timing_cur;
            }
            if (dev->sm.timing_totalcnt > 100000)
            { /* auto reset */
                dev->sm.timing_timer = -1;
            }
        }
    }
    if (dev->sm.tick_timer++ >= 10)
    { /* 1s */
        dev->sm.tick_timer = 0;
        dev->sm.tick++;
    }

    switch (sta_get_state())
    {
    case SMST_LAUNCH:
        sta_sm_launch();
        break;

    case SMST_STDBY:
        sta_sm_stdby();
        break;

    case SMST_STOP:
        sta_sm_stop();
        break;

    case SMST_READY:
        sta_sm_ready();
        break;

    case SMST_CHG:
        sta_sm_chg();
        break;

    case SMST_DHG:
        sta_sm_dhg();
        break;

    case SMST_ERR:
        sta_sm_err();
        break;

    default:
        break;
    }
}

static void *sta_thrd(void *param)
{
    log_dbg("%s, ++", __func__);

    while (1)
    {
        sta_sm();
        usleep(100000); /* 100ms */
    }

    log_dbg("%s, --", __func__);
}

int sta_init(void)
{
    pthread_t thrd;
    char buf[32];
    int result;
    char *errmsg = NULL;
    sqlite3 *db = plt_get_projdb();
    char sql[1024];
    struct dbcbparam_t cbparam;
    int ret = 0;
    int i;
    struct mdl_t *dev = &mdl;

    log_dbg("%s, ++", __func__);

    sprintf(sql, "select * from sta");
    cbparam.nrow = 0;
    plt_lock_projdb();
    result = sqlite3_exec(db, sql, sta_dbcb_0, (void *)&cbparam, &errmsg);
    plt_unlock_projdb();
    if (result != SQLITE_OK)
    {
        log_dbg("%s, result != SQLITE_OK, cause : %s", __func__, errmsg);
        ret = -1;
    }
    else if (cbparam.ret != 0)
    {
        log_dbg("%s, cbparam.ret != 0", __func__);
        ret = -1;
    }
    else
    {
        if (cbparam.nrow != 1)
        {
            log_dbg("%s, cbparam.nrow(%d) != 1", __func__, cbparam.nrow);
            ret = -1;
        }
        else
        {
            sta_set_state(SMST_LAUNCH, SMERR_NONE);
            dev->sm.tick_timer = 0;
            dev->sm.timing_timer = -1;

            dev->pow.norm_cap = 0.0;
            dev->pow.norm_pow = 0.0;
            sta_set_dhgable();
            sta_set_chgable();

            if (pthread_create(&thrd, NULL, sta_thrd, NULL) != 0)
            {
                log_dbg("%s, create sta_thrd_sm fail", __func__);
                ret = -1;
            }
        }
    }

    log_dbg("%s, --, ret:%d", __func__, ret);
    return ret;
}

int sta_get_init_data(int idx, char *buf)
{
    sprintf(buf, "{\"device_id\":\"%s\", \"type\":0, \"idx\":%d, \"config\":{\"key\":\"test\"}}",
            mdl.szprojId, idx);

    return 0;
}