#ifndef CLOUD_H
#define CLOUD_H

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
int cloud_enable_upload();
void cloud_exe( void );

#endif /* CLOUD_H */
