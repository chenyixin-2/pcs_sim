#ifndef MQTT_H
#define MQTT_H
#include "plt.h"

struct mqtt_t
{
    int dbg;
    MQTTClient cli;
    int enable;
    char szservip[32];
    int servport;
    char szclientid[150];
    char szaccesstoken[64];
    char szusername[64];
    char szpasswd[64];
    char sztopic[128];
    int timezone;
    int connlost;
    int conncnt;
    MQTTClient_deliveryToken token;
    MQTTClient_connectOptions conn_opts;
    int cmd;

    double txbuf_usage; /* 0-100 */
    double rxbuf_usage; /* 0-100 */

    double pub_starttime;
    double pub_endtime;
    double pub_totaltime; //
    double pub_totalcnt;
    double pub_max;
    double pub_ave;
    int pub_failed;          // 当前连续失败了多少次
    double pub_totalFailcnt; // 总共失败了多少次
    double pub_maxFailcnt;   // 连续失败多少次
    // int tick;

    struct statemachine_t sm;
    struct comm_t comm;
};

extern struct mqtt_t mqtt;

int mqtt_init(void);
int mqtt_set_state(int state, int errcode);
int mqtt_get_state();
int mqtt_send_sm_cmd(int cmd);
int mqtt_get_cmd();
void mqtt_reset_cmd();
void mqtt_lock_txbuf();
void mqtt_unlock_txbuf();
int mqtt_get_rb_used(void);
int mqtt_get_rb_size(void);
void mqtt_init_txbuf();
void mqtt_lock_txbuf();
void mqtt_unlock_txbuf();
void mqtt_queue_txbuf(mqtt_ringbuffer_element_t data);
int mqtt_dequeue_txbuf(mqtt_ringbuffer_element_t *data);
int mqtt_peek_txbuf(mqtt_ringbuffer_element_t *data, mqtt_ringbuffer_size_t index);
int mqtt_get_txbuf_used(void);
int mqtt_get_txbuf_size(void);

void mqtt_init_rxbuf();
void mqtt_lock_rxbuf();
void mqtt_unlock_rxbuf();
void mqtt_queue_rxbuf(mqtt_ringbuffer_element_t data);
int mqtt_dequeue_rxbuf(mqtt_ringbuffer_element_t *data);
int mqtt_peek_rxbuf(mqtt_ringbuffer_element_t *data, mqtt_ringbuffer_size_t index);
int mqtt_get_rxbuf_used(void);
int mqtt_get_rxbuf_size(void);
int mqtt_connect(void);
int mqtt_pub(char *sztopic, char *szpayload);
int mqtt_get_tz();

char *mqtt_get_state_str(void);
int mqtt_get_stp(void);
char *mqtt_get_err_str(void);
int mqtt_get_tick(void);
double mqtt_get_timing_ave(void);
double mqtt_get_timing_cur(void);
double mqtt_get_timing_max(void);
int mqtt_get_enable(void);
char *mqtt_get_servip_str(void);
int mqtt_get_servport(void);
char *mqtt_get_client_id(void);
double mqtt_get_txbuf_usage(void);
char *mqtt_get_access_token(void);
int mqtt_get_tool_data(char *buf);
int mqtt_set_dbg(int val);
#endif /* MQTT_H */
