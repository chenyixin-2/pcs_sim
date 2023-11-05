#ifndef __STA_H__
#define __STA_H__
int sta_init( void );
int sta_send_cmd( int cmd );
int sta_set_aps(int val);
int sta_get_ap( );
int sta_get_aps( );
double sta_get_soc( );
int sta_chk_chg_stat( void );
int sta_set_mode( int mod );
int sta_chk_all_ctn_stat(int stat);
int sta_chk_all_meter_stat(int stat);
int sta_chk_any_meter_stat(int stat);
int sta_send_all_ctn_cmd( int cmd );
int sta_send_all_meter_cmd( int cmd );
int sta_dis_pow( void );
int sta_chk_dhg_stat(void);
int sta_chk_chg_stat( void );
char* sta_get_prjid( void );
char *sta_get_devid(void);
char* sta_get_stat_str();
char* sta_get_err_str();
void sta_set_dhgable( void );
void sta_reset_dhgable( void );
void sta_set_chgable( void );
void sta_reset_chgable( void );
int sta_get_dhgable( void );
int sta_get_chgable( void );
int sta_get_timezone();
char* sta_get_state_str();
char* sta_get_err_str();
int sta_get_norm_cap();
int sta_get_norm_pow();
#endif
