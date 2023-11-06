#ifndef SM_H
#define SM_H

struct err_t{
    int code;
    char szstr[128];
};

struct state_t{
    int code;
    char szstr[128];
};

struct statemachine_t{
    struct state_t* states;
    int state_nbr;
    struct err_t* errs;
    int err_nbr;
    int state;
    char szstate[16];
    int step;
    int count;
    int err;
    char szerrcode[64];
    
    unsigned char tick;
    int tick_timer;
    int tick_prd;

    /* timing statistics */
    int timing_timer;
    int timing_prd;
    double timing_tslastrun;
    double timing_totalcnt;
    double timing_totaltime;
    double timing_ave;
    double timing_max;   
    double timing_cur; 
};

void sm_set_state(struct statemachine_t* sm, int state, int err);
int sm_get_state(struct statemachine_t* sm);
char* sm_get_szstate(struct statemachine_t* sm);
void sm_reset_timing(struct statemachine_t* sm, int timing_prd, int tick_prd);
void sm_cal_timing(struct statemachine_t* sm);
int sm_get_step(struct statemachine_t* sm);
void sm_set_step(struct statemachine_t* sm, int step);
int sm_get_count(struct statemachine_t* sm);
void sm_set_count(struct statemachine_t* sm, int val);
void sm_inc_count(struct statemachine_t* sm);
void sm_get_summary(struct statemachine_t* sm, char* buf, int len);
double sm_get_timeofday();
#endif /* SM_H */
