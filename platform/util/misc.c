#include "plt.h"

/* Bit-mask values for 'flags' argument of becomeDaemon() */

#define BD_NO_CHDIR 01          /* Don't chdir("/") */
#define BD_NO_CLOSE_FILES 02    /* Don't close all open files */
#define BD_NO_REOPEN_STD_FDS 04 /* Don't reopen stdin, stdout, and \
                                   stderr to /dev/null */
#define BD_NO_UMASK0 010        /* Don't do a umask(0) */

#define BD_MAX_CLOSE 8192 /* Maximum file descriptors to close if \
                             sysconf(_SC_OPEN_MAX) is indeterminate */

int misc_createDir(const char *sPathName)
{
    char DirName[256];
    strcpy(DirName, sPathName);
    int i, len = strlen(DirName);

    if (DirName[len - 1] != '/')
        strcat(DirName, "/");

    len = strlen(DirName);

    for (i = 1; i < len; i++)
    {
        if (DirName[i] == '/')
        {
            DirName[i] = 0;
            if (access(DirName, 0755) != 0)
            {
                if (mkdir(DirName, 0755) == -1)
                {
                    perror("mkdir   error");
                    return -1;
                }
            }
            DirName[i] = '/';
        }
    }

    return 0;
}

double misc_gettimeofday()
{
    struct timeval tv;
    struct timezone tz;
    double starttime;
    double endtime;
    gettimeofday(&tv, &tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

int misc_get_datetime(int *y, int *m, int *d, int *h, int *min, int *s)
{
    time_t timep;
    struct tm *tsp;
    int ret = 0;

    time(&timep);
    // tsp = gmtime(&timep);
    tsp = localtime(&timep);

    *y = 1900 + tsp->tm_year;
    *m = 1 + tsp->tm_mon;
    *d = tsp->tm_mday;
    *h = tsp->tm_hour;
    *min = tsp->tm_min;
    *s = tsp->tm_sec;

    return ret;
}

int misc_day_diff(int year_start, int month_start, int day_start, int year_end, int month_end, int day_end)
{
    int y2, m2, d2;
    int y1, m1, d1;

    m1 = (month_start + 9) % 12;
    y1 = year_start - m1 / 10;
    d1 = 365 * y1 + y1 / 4 - y1 / 100 + y1 / 400 + (m1 * 306 + 5) / 10 + (day_start - 1);

    m2 = (month_end + 9) % 12;
    y2 = year_end - m2 / 10;
    d2 = 365 * y2 + y2 / 4 - y2 / 100 + y2 / 400 + (m2 * 306 + 5) / 10 + (day_end - 1);

    return (d2 - d1);
}

void misc_gen_datetimestr(char *buf, int len)
{
    time_t timep;
    struct tm *tsp;
    char tmpbuf[32];

    time(&timep);
    // tsp = gmtime(&timep);
    tsp = localtime(&timep);
    sprintf(tmpbuf, "%04d-%02d-%02d %02d:%02d:%02d", tsp->tm_year + 1900,
            tsp->tm_mon + 1,
            tsp->tm_mday,
            tsp->tm_hour,
            tsp->tm_min,
            tsp->tm_sec);
    strncpy(buf, tmpbuf, len);
}

void misc_gen_timestring(char *buf, int len)
{
    time_t timep;
    struct tm *tsp;
    char tmpbuf[32];

    time(&timep);
    // tsp = gmtime(&timep);
    tsp = localtime(&timep);

    sprintf(tmpbuf, "%02d:%02d:%02d", tsp->tm_hour, tsp->tm_min, tsp->tm_sec);

    strncpy(buf, tmpbuf, len);
}

void misc_get_datetimestr(char *buf, int len)
{
    time_t timep;
    struct tm *tsp;
    char tmpbuf[32];

    time(&timep);
    // tsp = gmtime(&timep);
    tsp = localtime(&timep);
    sprintf(tmpbuf, "%04d-%02d-%02d %02d:%02d:%02d", tsp->tm_year + 1900,
            tsp->tm_mon + 1,
            tsp->tm_mday,
            tsp->tm_hour,
            tsp->tm_min,
            tsp->tm_sec);
    strncpy(buf, tmpbuf, len);
}

unsigned char CalChecksum(unsigned char *psrc, int count)
{
    unsigned char checksum = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        checksum += psrc[i];
    }
    return (checksum);
}

unsigned int calChecksum32(unsigned char *psrc, int count)
{
    unsigned int checksum = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        checksum += psrc[i];
    }
    return (checksum);
}

unsigned short calChecksum16(unsigned char *psrc, int count)
{
    unsigned short checksum = 0;
    int i;
    for (i = 0; i < count; i++)
    {
        checksum += psrc[i];
    }
    return (checksum);
}

#if 0
static void dump(unsigned char* buf, int len)
{
    char buf_s[1024];
    char buf_t[16];
    int i;

    memset(buf_s, 0, sizeof(buf_s));
    //strcat(buf_s,"\n");
    for(i = 0; i<len; i++){
        sprintf(buf_t,"%02X ",buf[i]);
        strcat(buf_s,buf_t);
    }
    strcat(buf_s,"\n");
    syslog(LOG_INFO,"%s",buf_s);
}
#else
void dump(char *buf_dst, unsigned char *buf_src, int len)
{
    char buf_t[16];
    int i;

    buf_dst[0] = 0;
    for (i = 0; i < len; i++)
    {
        sprintf(buf_t, "%02X ", buf_src[i]);
        strcat(buf_dst, buf_t);
    }
}
#endif

// 0 : big end
// 1 : little end
int JudgeEnd(void)
{
    int num = 1;

    // *((char*)&num)���num������ֽڣ�Ϊ0x00,˵���Ǵ�� Ϊ0x01,˵����С��
    return *((char *)&num) ? 1 : 0;
}

// little end
// val -> short -> unsigned short
// NOTE:modbus use big end
unsigned short f32_to_u16(float val)
{
    short stmp;
    unsigned short ret;

    stmp = (short)val;
    memcpy((unsigned char *)&ret, (unsigned char *)&stmp, 2);
    return ret;
}

void f32_to_pi32(unsigned char *pdst, float fval)
{
    int itmp = (int)fval;
    *pdst++ = itmp & 0xFF;
    *pdst++ = (itmp >> 8) & 0xFF;
    *pdst++ = (itmp >> 16) & 0xFF;
    *pdst++ = (itmp >> 24) & 0xFF;
}

void f32_to_pi16(unsigned char *pdst, float fval)
{
    short itmp = (int)fval;
    *pdst++ = itmp & 0xFF;
    *pdst++ = (itmp >> 8) & 0xFF;
}

void i32_to_pu8(unsigned char *pdst, int ival)
{
    unsigned int utmp = (unsigned int)ival;
    *pdst++ = utmp & 0xFF;
    *pdst++ = (utmp >> 8) & 0xFF;
    *pdst++ = (utmp >> 16) & 0xFF;
    *pdst++ = (utmp >> 24) & 0xFF;
}

// little end
short pu8_to_i16(unsigned char *pval)
{
    return (short)(*pval | (*(pval + 1)) << 8);
}

// little end
int pu8_to_i32(unsigned char *pval)
{
    return (int)(*pval | (*(pval + 1)) << 8 | (*(pval + 2)) << 16 | (*(pval + 3)) << 24);
}

int file_size(char *filename)
{
    struct stat statbuf;
    stat(filename, &statbuf);
    int size = statbuf.st_size;

    return size;
}

/*
 * System function daemon() replacement based on FreeBSD implementation.
 * Original source file CVS tag:
 * $FreeBSD: src/lib/libc/gen/daemon.c,v 1.3 2000/01/27 23:06:14 jasone Exp $
 */
int misc_daemon(int nochdir, int noclose)
{
    int fd;

    switch (fork())
    {
    case -1:
        return (-1);
    case 0:
        break;
    default:
        _exit(0);
    }

    if (setsid() == -1)
        return (-1);

    if (!nochdir)
        (void)chdir("/");

    if (!noclose && (fd = open("/dev/null", O_RDWR, 0)) != -1)
    {
        (void)dup2(fd, STDIN_FILENO);
        (void)dup2(fd, STDOUT_FILENO);
        (void)dup2(fd, STDERR_FILENO);
        if (fd > 2)
            (void)close(fd);
    }
    return (0);
}

/* Returns 0 on success, -1 on error */
int misc_becomeDaemon(int flags)
{
    int maxfd, fd;

    switch (fork())
    { /* Become background process */
    case -1:
        return -1;
    case 0:
        break; /* Child falls through... */
    default:
        _exit(EXIT_SUCCESS); /* while parent terminates */
    }

    if (setsid() == -1) /* Become leader of new session */
        return -1;

    switch (fork())
    { /* Ensure we are not session leader */
    case -1:
        return -1;
    case 0:
        break;
    default:
        _exit(EXIT_SUCCESS);
    }

    if (!(flags & BD_NO_UMASK0))
        umask(0); /* Clear file mode creation mask */

    if (!(flags & BD_NO_CHDIR))
        chdir("/"); /* Change to root directory */

    if (!(flags & BD_NO_CLOSE_FILES))
    { /* Close all open files */
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1)          /* Limit is indeterminate... */
            maxfd = BD_MAX_CLOSE; /* so take a guess */

        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    if (!(flags & BD_NO_REOPEN_STD_FDS))
    {
        close(STDIN_FILENO); /* Reopen standard fd's to /dev/null */

        fd = open("/dev/null", O_RDWR);

        if (fd != STDIN_FILENO) /* 'fd' should be 0 */
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}
