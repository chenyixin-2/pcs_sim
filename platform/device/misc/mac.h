#ifndef __MAC_H__
#define __MAC_H__

struct mac_t{
    float cpu_occupy;
    float mem_occupy;
    float disk_occupy;
    int cal_timer;
    int cal_intv;
};

struct cpu_info_t{
    char name[20];             //定义一个char类型的数组名name有20个元素
    unsigned int user;        //定义一个无符号的int类型的user
    unsigned int nic;        //定义一个无符号的int类型的nice
    unsigned int system;    //定义一个无符号的int类型的system
    unsigned int idle;         //定义一个无符号的int类型的idle
    unsigned int iowait;
    unsigned int irq;
    unsigned int softirq;
};

int mac_init( void );
float mac_get_cpu_occupy();
void mac_get_cpu_info(struct cpu_info_t* cpu_info);
float mac_cal_cpu_occupy(struct cpu_info_t *first, struct cpu_info_t*second);
float mac_get_mem_occupy();
float mac_get_disk_occupy();
#endif