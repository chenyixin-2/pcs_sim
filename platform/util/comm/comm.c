#include "plt.h"

#define NONE "\e[0m"
#define BLACK "\e[0;30m"
#define L_BLACK "\e[1;30m"
#define RED "\e[0;31m"
#define L_RED "\e[1;31m"
#define GREEN "\e[0;32m"
#define L_GREEN "\e[1;32m"
#define BROWN "\e[0;33m"
#define YELLOW "\e[1;33m"
#define BLUE "\e[0;34m"
#define L_BLUE "\e[1;34m"
#define PURPLE "\e[0;35m"
#define L_PURPLE "\e[1;35m"
#define CYAN "\e[0;36m"
#define L_CYAN "\e[1;36m"
#define GRAY "\e[0;37m"
#define WHITE "\e[1;37m"

#define BOLD "\e[1m"
#define UNDERLINE "\e[4m"
#define BLINK "\e[5m"
#define REVERSE "\e[7m"
#define HIDE "\e[8m"
#define CLEAR "\e[2J"
#define CLRLINE "\r\e[K"

/* ms */
static double comm_get_timeofday()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

void comm_set_state(struct comm_t *comm, int state)
{
    comm->state = state;
    if (state == COMMST_NORMAL)
    {
        strcpy(comm->szState, "norm");
    }
    else if (state == COMMST_ERR)
    {
        strcpy(comm->szState, "err");
    }
    else
    {
        strcpy(comm->szState, "unknown");
    }
}

int comm_get_state(struct comm_t *comm)
{
    return comm->state;
}

void comm_set_dac_param_en(struct comm_t *comm, int val)
{
    comm->dac.param_en = val;
}

int comm_get_dac_param_en(struct comm_t *comm)
{
    return comm->dac.param_en;
}

void comm_start_cal_dac_timing(struct comm_t *comm)
{
    comm->dac.timing_start = comm_get_timeofday();
}

void comm_stop_cal_dac_timing(struct comm_t *comm)
{
    if (comm->dac.totalcnt < -0.1)
    { /* reset */
        comm->dac.timing_ave = -1.0;
        comm->dac.timing_max = -1.0;
        comm->dac.timing_cur = -1.0;
        comm->dac.totalcnt = 0.0;
        comm->dac.totaltime = 0.0;
    }
    else
    {
        comm->dac.timing_end = comm_get_timeofday();
        comm->dac.timing_cur = comm->dac.timing_end - comm->dac.timing_start;
        comm->dac.totalcnt += 1;
        comm->dac.totaltime += comm->dac.timing_cur;
        comm->dac.timing_ave = comm->dac.totaltime / comm->dac.totalcnt;
        if (comm->dac.timing_cur > comm->dac.timing_max)
        {
            comm->dac.timing_max = comm->dac.timing_cur;
        }
        if (comm->dac.totalcnt > 1640000)
        { /* auto reset */
            comm->dac.totalcnt = -1.0;
        }
    }
}

void comm_get_summary(struct comm_t *comm, char *buf, int len)
{
    sprintf(buf, "chan:%d adr:%d state:%s dac_timing_ave(ms):%.0f max:%.0f cur:%.0f",
            comm->chanidx, comm->adr,
            comm->szState,
            comm->dac.timing_ave, comm->dac.timing_max, comm->dac.timing_cur);
}

void comm_reset(struct comm_t *comm)
{
    comm_set_state(comm, COMMST_NORMAL);
    comm_set_dac_param_en(comm, 1);
}

int comm_get_adr(struct comm_t *comm)
{
    return comm->adr;
}

int comm_get_chan_idx(struct comm_t *comm)
{
    return comm->chanidx;
}

char *comm_get_state_str(struct comm_t *comm)
{
    return comm->szState;
}