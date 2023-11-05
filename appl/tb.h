#ifndef __TB_H__
#define __TB_H__
struct tb_t
{
    int upload_enable;

    int param_en;

    int sys_timer[4];
    int sys_intv[4];

    int meter_timer[4]; /* 0:60s */
    int meter_intv[4];

    int ems_timer[2]; // 0 - 60s
    int ems_intv[2];

    int tb_lock;
    int tb_lock_timer;
    int tb_lock_intv;
};

extern struct tb_t tb;

int tb_init();
void tb_exe(void);
int tb_set_param_en(int val);
int tb_set_cellv_highspeed();
int tb_set_cellv_lowspeed();

char *tb_get_state_str(void);
int tb_get_stp(void);
char *tb_get_err_str(void);
int tb_get_tick(void);
double tb_get_timing_ave(void);
double tb_get_timing_cur(void);
double tb_get_timing_max(void);
int tb_get_enable(void);
char *tb_get_servip_str(void);
int tb_get_servport(void);
char *tb_get_client_id(void);
double tb_get_txbuf_usage(void);
char *tb_get_access_token(void);
int tb_get_tz(void);
int tb_get_tool_data(char *buf);
long long tb_get_ts();
#endif
