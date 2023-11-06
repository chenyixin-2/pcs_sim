#include "plt.h"

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"

/* ms */
double sm_get_timeofday()
{
    struct  timeval    tv;
    struct  timezone   tz;
 
    gettimeofday(&tv,&tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec/1000;  
}

void sm_reset_timing(struct statemachine_t* sm, int timing_prd, int tick_prd)
{
    sm->timing_timer = -1;
    sm->timing_prd = timing_prd;
    sm->timing_ave = -1.0;
    sm->timing_max = -1.0;        
    sm->timing_cur = -1.0;

    sm->tick_timer = 0;
    sm->tick = 0;
    sm->tick_prd = tick_prd;
}

void sm_cal_timing(struct statemachine_t* sm)
{
    /* state machine timing statistics */
    double tseclipsed = 0.0;
    double ts = 0.0;
    if( sm->timing_timer < 0 ){ /* reset */
        sm->timing_timer = 0;
        sm->timing_tslastrun = sm_get_timeofday();
        sm->timing_totalcnt = 0.0;
        sm->timing_totaltime = 0.0;
        sm->timing_ave = -1.0;
        sm->timing_max = -1.0;        
        sm->timing_cur = -1.0;
    }else{
        if( ++sm->timing_timer >= sm->timing_prd ){ /* cal every prd times */
            sm->timing_timer = 0;
            ts = sm_get_timeofday();
            tseclipsed = ts - sm->timing_tslastrun;
            sm->timing_totalcnt += sm->timing_prd;
            sm->timing_totaltime += tseclipsed;
            sm->timing_tslastrun = ts;
            sm->timing_ave = sm->timing_totaltime/sm->timing_totalcnt;
            sm->timing_cur = tseclipsed/sm->timing_prd;
            if( sm->timing_cur > sm->timing_max ){
                sm->timing_max = sm->timing_cur;
            }      
            if( sm->timing_totalcnt > 8640000 ){ /* auto reset */
                sm->timing_timer = -1;
            }      
        }

    }
    
    if( sm->tick_timer++ >= sm->tick_prd ){ //
        sm->tick_timer = 0;
        sm->tick++;    
    }    
}

void sm_set_state(struct statemachine_t* sm, int state, int err)
{
    int i, found;

    sm->state = state;
    sm->step = 0;
    sm->count = 0;

    found = 0;
    for( i = 0; i < sm->state_nbr; i++ ){
        if( state == sm->states[i].code ){
            found = 1;
            strcpy(sm->szstate, sm->states[i].szstr);
        }
    }
    if( found != 1 ){
        strcpy(sm->szstate, "unknown");
    }

    found = 0;
    for( i = 0; i < sm->err_nbr; i++ ){
        if( err == sm->errs[i].code ){
            found = 1;
            strcpy(sm->szerrcode, sm->errs[i].szstr);
        }
    }
    if( found != 1 ){
        strcpy(sm->szerrcode, "unknown");
    }
}

int sm_get_state(struct statemachine_t* sm)
{
    return sm->state;
}

char* sm_get_szstate(struct statemachine_t* sm)
{
    return sm->szstate;
}

int sm_get_step(struct statemachine_t* sm)
{
    return sm->step;
}

void sm_set_step(struct statemachine_t* sm, int step)
{
    sm->step = step;
}

int sm_get_count(struct statemachine_t* sm)
{
    return sm->count;
}

void sm_set_count(struct statemachine_t* sm, int val)
{
    sm->count = val;
}

void sm_inc_count(struct statemachine_t* sm)
{
    sm->count++;
}

void sm_get_summary(struct statemachine_t* sm, char* buf, int len)
{
    sprintf(buf, "SM stat:" YELLOW "%s" NONE " stp:%d err:[%s] tick:%03d timing_ave(ms):%.0f cur:%.0f max:%.0f",
        sm->szstate, sm->step, sm->szerrcode, sm->tick, sm->timing_ave, sm->timing_cur, sm->timing_max);
}
