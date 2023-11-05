#ifndef __CLOUD_H__
#define __CLOUD_H__

struct cloud_t{
    int sys_timer[4];
    int sys_intv[4];
    
    int meter_timer[2];
    int meter_intv[2];    
    int upload_enable;
};

long long cloud_get_timezonets();
long long cloud_get_unixts();

int cloud_init();
void cloud_exe( void );
void cloud_upload_init_data(char *buf);

#endif