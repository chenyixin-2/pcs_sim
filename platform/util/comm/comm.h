#ifndef COMM_H
#define COMM_H

enum dat_st_t{
    DACST_STARTED = 0,
    DACST_FINISHED,
};

struct dac_t{
    int state;
    int stp;
    int timer;
    int timeout;
    int updated;

    int param_en;

    double totalcnt;
    double totaltime;
    double timing_ave;
    double timing_max;
    double timing_cur;
    double timing_start;
    double timing_end;

};

enum commst_t{
    COMMST_NORMAL = 0,
    COMMST_ERR,
};

struct comm_t{
    int chanidx; 
    int adr; 
    int state;
    char szState[32];
    struct dac_t dac;   
};

void comm_reset(struct comm_t* comm);
void comm_set_state(struct comm_t* comm, int state);
int comm_get_state( struct comm_t* comm );
void comm_set_dac_param_en(struct comm_t* comm, int val);
int comm_get_dac_param_en(struct comm_t* comm);
void comm_start_cal_dac_timing(struct comm_t* comm);
void comm_stop_cal_dac_timing(struct comm_t* comm);
void comm_get_summary(struct comm_t* comm, char* buf, int len);
int comm_get_adr(struct comm_t* comm);
int comm_get_chan_idx(struct comm_t* comm);
char* comm_get_state_str(struct comm_t* comm);
#endif /* COMM_H */
