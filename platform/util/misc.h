#ifndef __MISC_H__
#define __MISC_H__

#define EPS (0.000001)
#define ISZERO(x) (((x<EPS)&&(x>-EPS))?1:0)

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K"

//use JudgeEnd()
#define BYTEP2INT(a)    ((int)((((unsigned char*)(a))[0]) | (((unsigned char*)(a))[1])<<8 | (((unsigned char*)(a))[2])<<16 | (((unsigned char*)(a))[3])<<24))
//#define pu8_to_i16(a)    ((short)((((unsigned char*)(a))[0]) | (((unsigned char*)(a))[1])<<8))

#define max(a,b) ( ((a)>(b)) ? (a):(b) )
#define min(a,b) ( ((a)>(b)) ? (b):(a) )

struct tick_t{
    int en;
    int chken;
    int timer;
    int chkcnt;
    int timeout;
    unsigned char to_dev;
    unsigned char from_host; /* increase 1 every second when dhg or chg by host */
    unsigned char last_from_host;
    unsigned char from_dev;
};

struct power_t{
    double soc;
    double soh;
    int active_p; // kW
    int active_p_set;
    int last_active_p_set;

    int norm_cap;
    int norm_pow;
    int min_pow;

    int bdhgable;
    int bchgable;
};

struct statemachine_t{
    int state;
    char szstate[16];
    int step;
    int count;
    int errcode;
    char szerrcode[64];
    unsigned char tick;
    int tick_timer;

    /* timing statistics */
    int timing_timer;
    double tslastrun;
    double totalcnt;
    double totaltime;
    double ave;
    double max;
    double cur;
};

struct comm_t{
    int online;
    int pollstp;
    unsigned char tick;
    int pollfailcnt;
    int pollrstcnt;
    int pollupdated;
    int pollupdated_to;
    double polltotalfailcnt;
    //double polltotalrstcnt;
    double rstcnt;
    double polltotalcnt;
    double polltotaltime;
    double pollave;
    double pollmax;
    double pollcur;
    double pollstarttime;
    double pollendtime;

    int poll_param_en;

    int q_used;
    int q_size;

    /* timing statistics */
    /* in ctnmdl, this is used to cal modbus tcp write */
    int timing_timer;
    double tslastrun;
    double totalcnt;
    double totaltime;
    double ave;
    double max;
};

#define EPS (0.0001)
#define ISZERO(x) (((x<EPS)&&(x>-EPS))?1:0)

//
// some userful bit tricks
//
#define misc_resetbits(x,m)	((x) &= ~(m))
#define misc_setbits(x,m)	((x) |= (m))
#define misc_testbits(x,m)	((x) & (m))
#define misc_bitmask(b)	(1<<(b))
#define misc_bit2mask(b1,b2)	(misc_bitmask(b1) | misc_bitmask(b2))
#define misc_setbit(x,b)	    misc_setbits(x, misc_bitmask(b))
#define misc_resetbit(x,b)	misc_resetbits(x, misc_bitmask(b))
#define misc_testbit(x,b)	misc_testbits(x, misc_bitmask(b))
#define misc_set2bits(x,b1,b2)	misc_setbits(x, (misc_bit2mask(b1, b2)))
#define misc_reset2bits(x,b1,b2)	misc_resetbits(x, (misc_bit2mask(b1, b2)))
#define misc_test2bits(x,b1,b2)	misc_testbits(x, (misc_bit2mask(b1, b2)))

void dump(char* buf_s, unsigned char* buf, int len);
unsigned char CalChecksum8(unsigned char* psrc, int count);
unsigned short calChecksum16(unsigned char* psrc, int count);
unsigned int calChecksum32(unsigned char* psrc, int count);

void StateLed_SetNormal(void);
void StateLed_SetAlert(void);
void Task_StateLed(void);
int JudgeEnd(void);
unsigned short f32_to_u16(float val);
short pu8_to_i16(unsigned char* pval);
void f32_to_pi32(unsigned char* pdst,float fval);
void i32_to_pu8(unsigned char* pdst, int ival);
void f32_to_pi16(unsigned char* pdst,float fval);
int file_size(char* filename);
int misc_thrdPriority(void);
int misc_createDir(const char *sPathName);
int misc_get_datetime(int* y, int* m, int* d, int* h, int* min, int* s);
void misc_get_datetimestr(char* buf, int len);
int misc_becomeDaemon(int flags);
double misc_gettimeofday();
void misc_gen_datetimestr(char* buf, int len);
int misc_day_diff(int year_start, int month_start, int day_start, int year_end, int month_end, int day_end);
#endif
