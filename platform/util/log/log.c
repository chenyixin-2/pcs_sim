#include "plt.h"
#include "log.h"

zlog_category_t *zlog_c;

struct buf_t
{
    char buf[2048];
    int i;
};

/* in seconds */
static long long log_get_unixts()
{
    return (long long)time(NULL);
}

/* in ms */
static long long log_get_timezonets()
{
    return (log_get_unixts() + mqtt_get_tz() * (long long)3600) * (long long)1000;
}

static void log_putchar(struct buf_t *b, char c)
{
    b->buf[b->i++] = c;
}

static void printNum(struct buf_t *b, unsigned long num, int base)
{
    /* 递归结束条件 */
    if (num == 0)
    {
        return;
    }

    /* 继续递归 */
    printNum(b, num / base, base);

    /* 逆序打印结果 */
    log_putchar(b, "0123456789abcdef"[num % base]);
}

static void printDeci(struct buf_t *b, int dec)
{
    int num;

    /* 处理有符号整数为负数时的情况 */
    if (dec < 0)
    {
        log_putchar(b, '-');
        dec = -dec; // 该操作存在溢出风险:最小的负数没有对应的正数
    }

    /* 处理整数为时0的情况 */
    if (dec == 0)
    {
        log_putchar(b, '0');
        return;
    }
    else
    {
        printNum(b, dec, 10); // 打印十进制数
    }
}

static void printOct(struct buf_t *b, unsigned oct)
{
    if (oct == 0) // 处理整数为0的情况
    {
        log_putchar(b, '0');
        return;
    }
    else
    {
        printNum(b, oct, 8); // 打印8进制数
    }
}

static void printHex(struct buf_t *b, unsigned hex)
{
    if (hex == 0) // 处理整数为0的情况
    {
        log_putchar(b, '0');
        return;
    }
    else
    {
        printNum(b, hex, 16); // 打印十六进制数
    }
}

static void printAddr(struct buf_t *b, unsigned long addr)
{
    /* 打印前导"0x" */
    log_putchar(b, '0');
    log_putchar(b, 'x');

    /* 打印地址:格式和十六进制一样 */
    printNum(b, addr, 16);
}

static void printStr(struct buf_t *b, const char *str)
{
    int i = 0;

    while (str[i] != '\0')
    {
        log_putchar(b, str[i++]);
    }
}

static void printFloat(struct buf_t *b, double f)
{
    int temp;

    /* 先打印整数部分 */
    if (f < 0)
    {
        printStr(b, "-");
    }
    f = fabs(f);
    temp = (int)f;
    printNum(b, temp, 10);

    /* 分隔点 */
    log_putchar(b, '.');

    /* 打印小数部分 */
    f -= temp;
    if (f == 0)
    {
        /* 浮点型数据至少六位精度 */
        for (temp = 0; temp < 6; temp++)
        {
            log_putchar(b, '0');
        }
        return;
    }
    else
    {
        temp = (int)(f * 1000000);
        printNum(b, temp, 10);
    }
}

void log_to_mqtt(const char *s, ...)
{
    return;
    int i = 0;
    int j = 0;
    tbmqtt_ringbuffer_element_t e_tb;
    mqtt_ringbuffer_element_t e_cloud;
    char itm_buf[2048];
    char temp_buf[2048];

    va_list va_ptr;
    va_start(va_ptr, s);
    vsnprintf(temp_buf, 2048, s, va_ptr);
    va_end(va_ptr);

    sprintf(itm_buf, "'sys_log':'%s'", temp_buf);
    // sprintf(e.szpayload,"{'ts':%lld,'items':{%s}}", log_get_timezonets(), itm_buf);

    // tb
    //  sprintf(e_tb.szpayload,"{'ts':%lld,'values':{%s}}", log_get_unixts() * 1000, itm_buf);
    //  strcpy(e_tb.sztopic,"v1/devices/me/telemetry");
    //  tbmqtt_lock_txbuf();
    //  tbmqtt_queue_txbuf(e_tb);
    //  tbmqtt_unlock_txbuf();

    // cloud
    sprintf(e_cloud.szpayload, "{\"timestamp\":%lld,\"data\":[%s]}", log_get_timezonets(), itm_buf);
    sprintf(e_cloud.sztopic, "%s", "sequential");
    mqtt_lock_txbuf();
    mqtt_queue_txbuf(e_cloud);
    mqtt_unlock_txbuf();
    return;
}

int log_init()
{
    int rc;
    int ret = 0;

    // zlog_category_t *c;
    rc = zlog_init("../cfg/log.conf");
    if (rc)
    {
        syslog(LOG_INFO, "%s, init failed", __func__);
        ret = -1;
    }
    else
    {
        zlog_c = zlog_get_category("my_cat");
        if (!zlog_c)
        {
            syslog(LOG_INFO, "%s, zlog_get_category fail", __func__);
            zlog_fini();
            ret = -2;
        }
    }

    syslog(LOG_INFO, "%s,ret:%d", __func__, ret);
    return ret;
}