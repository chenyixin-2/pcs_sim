#ifndef __TBMQTT_H__
#define __TBMQTT_H__

struct tbmqtt_t{
    int dbg;
    MQTTClient cli;
    int enable;
    char szservip[32];
    int servport;
    char szclientid[64];
    char szaccesstoken[64];
    char szusername[64];
    char szpasswd[64];
    char sztopic[128];
    int connlost;
    int conncnt ;
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

    //int tick;

    struct statemachine_t sm;
    struct comm_t comm;
};

int tbmqtt_connect( void );
int tbmqtt_init(void);
int tbmqtt_reset();
int tbmqtt_set_state( int state, int errcode);
int tbmqtt_get_state();
int tbmqtt_get_cmd();
void tbmqtt_reset_cmd();
int tbmqtt_set_dbg(int val);
int tbmqtt_pub(char* sztopic, char* szpayload );

int tbmqtt_send_sm_cmd( int cmd);
void tbmqtt_lock_txbuf();
void tbmqtt_unlock_txbuf();
int tbmqtt_get_rb_used( void );
int tbmqtt_get_rb_size( void );
void tbmqtt_init_txbuf();
void tbmqtt_lock_txbuf();
void tbmqtt_unlock_txbuf();
void tbmqtt_queue_txbuf(tbmqtt_ringbuffer_element_t data);
int tbmqtt_dequeue_txbuf(tbmqtt_ringbuffer_element_t* data);
int tbmqtt_peek_txbuf(tbmqtt_ringbuffer_element_t* data, tbmqtt_ringbuffer_size_t index);
int tbmqtt_get_txbuf_used( void );
int tbmqtt_get_txbuf_size( void );

void tbmqtt_init_rxbuf();
void tbmqtt_lock_rxbuf();
void tbmqtt_unlock_rxbuf();
void tbmqtt_queue_rxbuf(tbmqtt_ringbuffer_element_t data);
int tbmqtt_dequeue_rxbuf(tbmqtt_ringbuffer_element_t* data);
int tbmqtt_peek_rxbuf(tbmqtt_ringbuffer_element_t* data, tbmqtt_ringbuffer_size_t index);
int tbmqtt_get_rxbuf_used( void );
int tbmqtt_get_rxbuf_size( void );
int tbmqtt_send_sm_cmd( int cmd);

char* tbmqtt_get_state_str(void);
int tbmqtt_get_stp(void);
char* tbmqtt_get_err_str(void);
int tbmqtt_get_tick(void);
double tbmqtt_get_timing_ave(void);
double tbmqtt_get_timing_cur(void);
double tbmqtt_get_timing_max(void);
int tbmqtt_get_enable(void);
char* tbmqtt_get_servip_str(void);
int tbmqtt_get_servport(void);
char* tbmqtt_get_client_id(void);
double tbmqtt_get_txbuf_usage(void);
char* tbmqtt_get_access_token(void);
int tbmqtt_get_tool_data(char* buf);
#endif