#include "plt.h"
#include <unistd.h>

static struct cpu_info_t cpu_info[2];

void mac_get_cpu_info(struct cpu_info_t* cpu_info)
{ 
	FILE 	*fd;
	char	buff[256];
	memset(buff, '\0', 256);
 
	fd = fopen("/proc/stat", "r");
	fgets(buff, sizeof(buff), fd);
 
	sscanf(buff, "%s %u %u %u %u", cpu_info->name, &cpu_info->user, &cpu_info->nic, 
				&cpu_info->system, &cpu_info->idle);
	fclose(fd);
}
 
float mac_cal_cpu_occupy(struct cpu_info_t *first, struct cpu_info_t*second)
{
	unsigned long	old_CPU_Time, new_CPU_Time;
	unsigned long	usr_Time_Diff, sys_Time_Diff, nic_Time_Diff;
	float 			cpu_use = 0.0;
 
	old_CPU_Time = (unsigned long)(first->user + first->nic + first->system + first->idle);
	new_CPU_Time = (unsigned long)(second->user + second->nic + second->system + second->idle);
 
	usr_Time_Diff = (unsigned long)(second->user - first->user);
	sys_Time_Diff = (unsigned long)(second->system - first->system);
	nic_Time_Diff = (unsigned long)(second->nic -first->nic);
 
	if ((new_CPU_Time - old_CPU_Time) != 0)
	  cpu_use = (float)100*(usr_Time_Diff + sys_Time_Diff + nic_Time_Diff)/(new_CPU_Time - old_CPU_Time);
	else
	  cpu_use = 0.0;
	return cpu_use;
}

/* unit:% */
float mac_get_cpu_occupy()
{
	float cpu_rate;
	struct cpu_info_t first, second;
	mac_get_cpu_info(&first);
	sleep(5);
	mac_get_cpu_info(&second);
 
	cpu_rate = mac_cal_cpu_occupy(&first, &second);
 
	return cpu_rate;
}


struct mem_info_t{
    char name[20];     
    unsigned long total;
    char name2[20];
};

/* unit:% */
float mac_get_mem_occupy()
{
	struct sysinfo tmp;
	int ret = 0;
    float free, total;
	ret = sysinfo(&tmp);
	if (ret == 0) {
        free = (unsigned long)tmp.freeram/(1024*1024);
        total = (unsigned long)tmp.totalram/(1024*1024);
        return (total - free)/total*100.0;
	} else {
		return -1.0;
	}
}

float mac_get_disk_occupy()
{
    FILE * fp;
    int h=0;
    char buffer[80],a[80],d[80],e[80],f[80],buf[256];
    double c,b;
    double dev_total=0,dev_used=0;
    float occupy;

    fp = popen("df","r");
    fgets(buf,256,fp);
    
    while(6==fscanf(fp,"%s %lf %lf %s %s %s",a,&b,&c,d,e,f)){
        dev_total+=b;
        dev_used+=c;
    }
    occupy = dev_used/dev_total*100;
    pclose(fp);
    return occupy;
}

int mac_init( void )
{
    int ret = 0;
    MDL.mac.cal_timer = 0;
    MDL.mac.cal_intv = 60;
    MDL.mac.cpu_occupy = 0.0;
    MDL.mac.mem_occupy = 0.0;
    MDL.mac.disk_occupy = 0.0;
    mac_get_cpu_info(&cpu_info[0]);

    log_dbg("%s, ret:%d", __func__, ret);
    return ret;
}


void mac_exe( void )
{
    if( MDL.mac.cal_timer++ >= MDL.mac.cal_intv ){
        MDL.mac.cal_timer = 0;
        mac_get_cpu_info(&cpu_info[1]);
        MDL.mac.cpu_occupy = mac_cal_cpu_occupy(&cpu_info[0], &cpu_info[1]);
        cpu_info[0] = cpu_info[1];
        MDL.mac.mem_occupy = mac_get_mem_occupy();
        //mdl.mac.disk_occupy = mac_get_disk_occupy();
    }
}