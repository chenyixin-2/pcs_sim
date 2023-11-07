#include "plt.h"
#include "port.h"

#define EXPORT_PATH "/sys/class/gpio/export"     // export文件路径
#define UNEXPORT_PATH "/sys/class/gpio/unexport" // unexport文件路径
#define DIR_OUT "out"
#define DIR_IN "in"

#define SA struct sockaddr
/*
#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_AND_READ_REGISTERS  0x17
*/

int channbr;
struct chan_t chan[CHAN_NBR_MAX + 1];

static struct chanthrd_param_t chanthrd_param[CHAN_NBR_MAX + 1];

void chan_lock(int idx)
{
    pthread_mutex_lock(&chan[idx].mutex);
}

void chan_unlock(int idx)
{
    pthread_mutex_unlock(&chan[idx].mutex);
}

static void chan_dump(char *buf_dst, unsigned char *buf_src, int len)
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

static int chan_set_baud(int fd, int speed)
{
    int i;
    int status;
    struct termios Opt;
    int speed_arr[] = {B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1200};
    int name_arr[] = {115200, 57600, 38400, 19200, 9600, 4800, 2400, 1200};

    tcgetattr(fd, &Opt);
    for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
    {
        if (speed == name_arr[i])
        {
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if (status != 0)
            {
                perror("tcsetattr fail\n");
                return -1;
            }
            else
            {
                return 0;
            }
        }
        tcflush(fd, TCIOFLUSH);
    }
    return -1;
}

static int chan_set_parity(int fd, int databits, int stopbits, int parity)
{
    struct termios options;
    if (tcgetattr(fd, &options) != 0)
    {
        perror("SetupSerial 1");
        return -1;
    }
    options.c_cflag &= ~CSIZE;
    switch (databits)
    {
    case 7:
        options.c_cflag |= CS7;
        break;
    case 8:
        options.c_cflag |= CS8;
        break;
    default:
        fprintf(stderr, "Unsupported data size\n");
        return -1;
    }
    switch (parity)
    {
    case 'n':
    case 'N':
        options.c_cflag &= ~PARENB;
        options.c_iflag &= ~INPCK;
        break;
    case 'o':
    case 'O':
        options.c_cflag |= (PARODD | PARENB);
        options.c_iflag |= INPCK;
        break;
    case 'e':
    case 'E':
        options.c_cflag |= PARENB;
        options.c_cflag &= ~PARODD;
        options.c_iflag |= INPCK;
        break;
    case 'S':
    case 's':
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported parity\n");
        return -1;
    }
    switch (stopbits)
    {
    case 1:
        options.c_cflag &= ~CSTOPB;
        break;
    case 2:
        options.c_cflag |= CSTOPB;
        break;
    default:
        fprintf(stderr, "Unsupported stop bits\n");
        return -1;
    }
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd, TCSANOW, &options) != 0)
    {
        perror("SetupSerial 3");
        return -1;
    }
    return 0;
}
int chan_open_serial(char *dev, int baud, int bits, char parity, int stop)
{
    struct termios tty;
    int serial_port = open(dev, O_RDWR);
    if (serial_port < 0)
    {
        log_dbg("open dev : %s fail, error:%i %s", dev, errno, strerror(errno));
        return -1;
    }

    // Read in existing settings, and handle any error
    if (tcgetattr(serial_port, &tty) != 0)
    {
        log_dbg("Error %i from tcgetattr: %s\n", errno, strerror(errno));
        close(serial_port);
        return -2;
    }

    switch (parity)
    {
    case 'O':
    case 'o':
        tty.c_cflag |= PARENB;
        tty.c_iflag |= INPCK;
        tty.c_cflag |= PARODD;
        break;

    case 'E':
    case 'e':
        tty.c_cflag |= PARENB;
        tty.c_iflag |= INPCK;
        tty.c_cflag &= ~PARODD;
        break;

    case 'N':
    case 'n':
        tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
        break;

    default:
        log_dbg("unsupported parity :%c\n", parity);
        close(serial_port);
        return -3;
    }

    if (stop != 1)
    {
        log_dbg("unsupported stop bit :%d\n", stop);
        close(serial_port);
        return -4;
    }
    else
    {
        tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    }

    switch (bits)
    {
    case 7:
        tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
        tty.c_cflag |= CS7;    // 8 bits per byte (most common)
        break;
    case 8:
        tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
        tty.c_cflag |= CS8;    // 8 bits per byte (most common)
        break;
    default:
        log_dbg("unsupported bit number:%d\n", bits);
        close(serial_port);
        return -5;
    }

    tty.c_cflag &= ~CRTSCTS;       // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;                                                        // Disable echo
    tty.c_lflag &= ~ECHOE;                                                       // Disable erasure
    tty.c_lflag &= ~ECHONL;                                                      // Disable new-line echo
    tty.c_lflag &= ~ISIG;                                                        // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                                      // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed
    // tty.c_oflag &= ~OXTABS; // Prevent conversion of tabs to spaces (NOT PRESENT ON LINUX)
    // tty.c_oflag &= ~ONOEOT; // Prevent removal of C-d chars (0x004) in output (NOT PRESENT ON LINUX)

    tty.c_cc[VTIME] = 10; // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 9600
    switch (baud)
    {
    case 1200:
        cfsetispeed(&tty, B1200);
        cfsetospeed(&tty, B1200);
        break;

    case 2400:
        cfsetispeed(&tty, B2400);
        cfsetospeed(&tty, B2400);
        break;

    case 4800:
        cfsetispeed(&tty, B4800);
        cfsetospeed(&tty, B4800);
        break;

    case 9600:
        cfsetispeed(&tty, B9600);
        cfsetospeed(&tty, B9600);
        break;

    case 19200:
        cfsetispeed(&tty, B19200);
        cfsetospeed(&tty, B19200);
        break;

    case 115200:
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);
        break;

    default:
        log_dbg("unsupported baud:%d\n", baud);
        close(serial_port);
        return -6;
    }

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0)
    {
        log_dbg("Error %i from tcsetattr: %s\n", errno, strerror(errno));
        close(serial_port);
        return -7;
    }
    return serial_port;
}

#define PF_CAN 29
#define AF_CAN PF_CAN
#define SIOCSCANBAUDRATE (SIOCDEVPRIVATE + 0)
#define SIOCGCANBAUDRATE (SIOCDEVPRIVATE + 1)
#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)
#define CAN_RAW_FILTER 1
#define CAN_RAW_RECV_OWN_MSGS 0x4
typedef __u32 can_baudrate_t;
struct ifreq ifr[CHAN_NBR_MAX + 1];

static struct chanthrd_param_t s_chanthrd_param[CHAN_NBR_MAX + 1];

static int chan_dbcb_0(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    int i;
    struct dbcbparam_t *pcbparam = (struct dbcbparam_t *)para;
    struct chan_t *ch;

    pcbparam->nrow++;
    log_dbg("%s, ++,row:%d;col:%d", __func__, pcbparam->nrow, ncolumn);

    ch = &chan[pcbparam->nrow];
    for (i = 0; i < ncolumn; i++)
    {
        if (strcmp("info", columnname[i]) == 0)
        {
            strcpy(ch->szinfo, columnvalue[i]);
        }
        else if (strcmp("dev", columnname[i]) == 0)
        {
            strcpy(ch->szdev, columnvalue[i]);
        }
        else if (strcmp("mode", columnname[i]) == 0)
        {
            strcpy(ch->szmode, columnvalue[i]);
            ch->mode = chan_mode_str2nbr(ch->szmode);
        }
        else if (strcmp("port", columnname[i]) == 0)
        {
            ch->port = atoi(columnvalue[i]);
        }
        else if (strcmp("baud", columnname[i]) == 0)
        {
            ch->baud = atoi(columnvalue[i]);
        }
        else if (strcmp("parity", columnname[i]) == 0)
        {
            strcpy(ch->szparity, columnvalue[i]);
        }
        else if (strcmp("servip", columnname[i]) == 0)
        {
            strcpy(ch->servip, columnvalue[i]);
        }
        else if (strcmp("servport", columnname[i]) == 0)
        {
            ch->servport = atoi(columnvalue[i]);
        }
        else if (strcmp("restart_times", columnname[i]) == 0)
        {
            ch->restart_times = atoi(columnvalue[i]);
        }
        else if (strcmp("retry_times", columnname[i]) == 0)
        {
            ch->retry_times = atoi(columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, --,ret:%d", __func__, pcbparam->ret);
    return 0;
}

static void *chan_socketcan_rxthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    int i = 0;
    int rc = 0;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t *ch = &chan[chanidx];
    int sock = ch->sockfd;
    int is_ext_frm = 0;
    unsigned int id = 0;
    chan_socketcan_ringbuffer_element_t e;
    char tmpbuf[256];

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while (1)
    {
        // usleep(2000);
        memset(&frame, 0, sizeof(struct can_frame));
        rc = read(sock, &frame, sizeof(struct can_frame));
        // if(rc > 0)
        // log_dbg("%s, read rc:%d, can_dlc:%d", __func__, rc, frame.can_dlc);
        if (frame.can_dlc)
        {
            // log_dbg("%s, read rc:%d, can_dlc:%d", __func__, rc, frame.can_dlc);
            if (ch->dbg)
            {
                is_ext_frm = (frame.can_id & CAN_EFF_FLAG) ? 1 : 0;
                if (is_ext_frm)
                {
                    id = frame.can_id & CAN_EFF_MASK;
                }
                else
                {
                    id = frame.can_id & CAN_SFF_MASK;
                }
                tmpbuf[0] = 0;
                dump(tmpbuf, frame.data, frame.can_dlc);
                log_dbg("%s, ID:0x%08X, data:%s", __func__, id, tmpbuf);
            }

            chan_lock(chanidx);
            e.frame = frame;
            chan_socketcan_rxrb_queue(chanidx, e);
            chan_unlock(chanidx);
        }
        else
        {
            usleep(2000);
        }
    }
    return NULL;
}

static void *chan_tcpservcan_rxthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    int i = 0;
    int rc = 0;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t *ch = &chan[chanidx];
    int sock = ch->sockfd;
    int is_ext_frm = 0;
    unsigned int id = 0;
    chan_tcpservcan_ringbuffer_element_t e;
    char tmpbuf[256];

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while (1)
    {
        memset(&frame, 0, sizeof(struct can_frame));
        rc = read(sock, &frame, sizeof(struct can_frame));
        if (frame.can_dlc)
        {
            if (ch->dbg)
            {
                is_ext_frm = (frame.can_id & CAN_EFF_FLAG) ? 1 : 0;
                if (is_ext_frm)
                {
                    id = frame.can_id & CAN_EFF_MASK;
                }
                else
                {
                    id = frame.can_id & CAN_SFF_MASK;
                }
                tmpbuf[0] = 0;
                dump(tmpbuf, frame.data, frame.can_dlc);
                log_dbg("%s, ID:0x%08X, data:%s", __func__, id, tmpbuf);
            }

            chan_lock(chanidx);
            e.frame = frame;
            chan_tcpservcan_rxrb_queue(chanidx, e);
            chan_unlock(chanidx);
        }
    }
    return NULL;
}

static void *chan_tcpservcan_txthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t *ch = &chan[chanidx];
    int sock = ch->sockfd;
    chan_tcpservcan_ringbuffer_element_t e_arr[32];
    int n2write;
    int ntry2write;
    int i;

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while (1)
    {
        // cnt = 0;
        chan_lock(chanidx);
        n2write = chan_tcpservcan_txrb_num_items(chanidx);
        if (n2write > 0)
        {
            ntry2write = n2write > (sizeof(e_arr) / sizeof(chan_tcpservcan_ringbuffer_element_t)) ? (sizeof(e_arr) / sizeof(chan_tcpservcan_ringbuffer_element_t)) : n2write;
            chan_tcpservcan_txrb_dequeue_arr(chanidx, e_arr, ntry2write);
        }
        else
        {
            ntry2write = 0;
        }
        chan_unlock(chanidx);
        if (ntry2write > 0)
        {
            for (i = 0; i < ntry2write; i++)
            {
                write(sock, &e_arr[i].frame, sizeof(struct can_frame));
            }
        }
        usleep(3000); /* 3ms */
    }
    return NULL;
}

static void *chan_socketcan_txthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t *ch = &chan[chanidx];
    int sock = ch->sockfd;
    chan_socketcan_ringbuffer_element_t e;
    int cnt = 0;

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    int nbytes = 0;
    while (1)
    {
        cnt = 3;
        chan_lock(chanidx);
        if (chan_socketcan_txrb_dequeue(chanidx, &e) != 0)
        {
            while (cnt--)
            {
                nbytes = write(sock, &e.frame, sizeof(chan_socketcan_ringbuffer_element_t));
                if (nbytes != sizeof(chan_socketcan_ringbuffer_element_t))
                {
                    log_info("%s,write err(%d)", __func__, nbytes);
                }
                else
                {
                    break;
                }
            }
        }
        chan_unlock(chanidx);
        usleep(1000);
    }
    return NULL;
}

static void *chan_serial_rxthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    int chan_idx = pparam->idx;
    struct chan_t *ch = &chan[chan_idx];
    int fd = ch->fd;
    chan_serial_ringbuffer_element_t e;
    int nread;
    char buff[128];
    char dump_buf[1024];
    int i;

    log_dbg("%s, ++, idx:%d, serial", __func__, chan_idx);
    while (true)
    {
        if ((nread = read(fd, buff, 100)) > 0)
        {
            if (ch->dbg > 0)
            {
                chan_dump(dump_buf, (unsigned char *)buff, nread);
                log_dbg("%s, idx:%d, rx %d bytes:%s", __func__, chan_idx, nread, dump_buf);
            }
            chan_lock(chan_idx);
            for (i = 0; i < nread; i++)
            {
                e.c = buff[i];
                chan_serial_rxrb_queue(chan_idx, e);
            }
            chan_unlock(chan_idx);
        }
        usleep(2000); /* 2ms */
    }
    return NULL;
}

static void *chan_serial_txthrd(void *thrdparam)
{
    struct chanthrd_param_t *pparam = (struct chanthrd_param_t *)thrdparam;
    int chan_idx = pparam->idx;
    struct chan_t *ch = &chan[chan_idx];
    int fd = ch->fd;
    chan_serial_ringbuffer_element_t e_arr[128];
    int towrite;
    int needtowrite;
    char buff[128];
    char dump_buf[1024];
    int i;

    log_dbg("%s, ++, idx:%d, serial", __func__, chan_idx);
    while (true)
    {
        chan_lock(chan_idx);
        needtowrite = chan_serial_txrb_num_items(chan_idx);
        if (needtowrite > 0)
        {
            towrite = needtowrite > sizeof(buff) ? sizeof(buff) : needtowrite;
            chan_serial_txrb_dequeue_arr(chan_idx, e_arr, towrite);
        }
        else
        {
            towrite = 0;
        }
        chan_unlock(chan_idx);
        if (towrite > 0)
        {
            for (i = 0; i < towrite; i++)
            {
                buff[i] = e_arr[i].c;
            }
            write(fd, buff, towrite);
            if (ch->dbg > 0)
            {
                chan_dump(dump_buf, (unsigned char *)buff, towrite);
                log_dbg("%s, idx:%d, tx %d bytes:%s", __func__, chan_idx, towrite, dump_buf);
            }
        }
        usleep(2000);
    }
    return NULL;
}

// void chan_set_mbs( int idx, int devm, int devidx )
//{
//     chan[idx].mbsdevm = devm;
//     chan[idx].mbsdevidx = devidx;
// }

int chan_set_nbr(int nbr)
{
    channbr = nbr;
    return 0;
}

int chan_get_nbr()
{
    return channbr;
}

int chan_init(void)
{
    int result = 0;
    int ret = 0;
    int i = 0;
    // int* channbr = &STA.channbr;
    struct chan_t *ch;
    char *errmsg = NULL;
    char sql[1024];
    struct dbcbparam_t cbparam;
    sqlite3 *db = NULL;

    log_dbg("%s, ++", __func__);

    plt_lock_cfgdb();
    db = plt_get_cfgdb();
    sprintf(sql, "select * from chan");
    cbparam.nrow = 0;
    result = sqlite3_exec(db, sql, chan_dbcb_0, (void *)&cbparam, &errmsg);
    plt_unlock_cfgdb();
    if (result != SQLITE_OK)
    {
        ret = -1;
    }
    else if (cbparam.ret != 0)
    {
        ret = -2;
    }
    else
    {
        chan_set_nbr(cbparam.nrow);
        for (i = 1; i <= chan_get_nbr(); i++)
        {
            ch = &chan[i];
            ch->errcnt = 0;
            ch->dbg = 0;
            ch->ctx = NULL;
            ch->started = 0;
            pthread_mutex_init(&ch->mutex, NULL);
            if (chan_start(i) < 0)
            {
                log_dbg("%s, chan idx:%d, start fail ", __func__, i);
            }
            // pthread_mutex_init(&ch->mutexdat, NULL);
            // if( chan_start(i) < 0 ){
            // }else{
            //     log_dbg("%s, idx:%d, start ok",__func__,i);
            // }
        }
    }

    log_dbg("%s--, ret:%d", __func__, ret);
    return ret;
}
// func code:0x06
int chan_write_single_register(int idx, int slaveaddr, int regaddr, int regval)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_write_register(ch->ctx, regaddr, regval);
        modbus_flush(ch->ctx);
        if (rc != 1)
        {
            ch->errcnt++;
            ret = -3;
        }
    }

    // log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, rc:%d, regval:%d",
    //                             __func__,idx,slaveaddr,regaddr,rc,regval);
    return ret;
}

// func code:0x06
int chan_write_single_register_with_retry(int idx, int slaveaddr, int regaddr, int regval)
{
    int rc;
    int ret = -1;
    struct chan_t *ch = &chan[idx];
    int retry_times = ch->retry_times;
    int restart_times = ch->restart_times;

    if (ch->ctx == NULL)
    {
        ret = -2;
    }
    else
    {
        while (restart_times-- > 0)
        {
            while (retry_times-- > 0)
            {
                if (ch->ctx != NULL)
                {
                    modbus_set_slave(ch->ctx, slaveaddr);
                    rc = modbus_write_register(ch->ctx, regaddr, regval);
                    modbus_flush(ch->ctx);
                    if (rc != 1)
                    {
                        ch->errcnt++;
                        usleep(100000); /* 100ms */
                    }
                    else
                    {
                        ret = 0; /* ok */
                        goto leave;
                    }
                }
            }
            usleep(100000); /* 100ms */
            chan_start(idx);
            retry_times = ch->retry_times;
        }
    }

leave:
    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, rc:%d, regval:%d",
                __func__, idx, slaveaddr, regaddr, rc, regval);
    }

    return ret;
}

// func code:0x10
int chan_write_multi_registers(int idx, int slaveaddr, int regaddr, int nb, unsigned short *regval)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_write_registers(ch->ctx, regaddr, nb, regval);
        modbus_flush(ch->ctx);
        if (rc != nb)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ch->dbg == 1)
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, rc:%d",
                __func__,
                idx,
                slaveaddr,
                regaddr,
                nb,
                rc);
    return ret;
}

// FUNC CODE: 0x02
int chan_read_input_bits(int idx, int slaveaddr, int regaddr, int nb, unsigned char *dest)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_read_input_bits(ch->ctx, regaddr, nb, dest);
        if (rc != nb)
        {
            _error_print(ch->ctx, __func__);
            ch->errcnt++;
            ret = -2;
        }
        modbus_flush(ch->ctx);
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, rc:%d",
                __func__,
                idx,
                slaveaddr,
                regaddr,
                nb,
                rc);
    }
    return rc;
}

int chan_read_input_bits_with_retry(int idx, int slaveaddr, int regaddr, int nb, unsigned char *dest)
{
    int rc;
    int ret = -1;
    struct chan_t *ch = &chan[idx];
    int retry_times = ch->retry_times;
    int restart_times = ch->restart_times;

    while (restart_times-- > 0)
    {
        while (retry_times-- > 0)
        {
            if (ch->ctx != NULL)
            {
                modbus_set_slave(ch->ctx, slaveaddr);
                rc = chan_read_input_bits(idx, slaveaddr, regaddr, nb, dest);
                modbus_flush(ch->ctx);
                if (rc != nb)
                {
                    ch->errcnt++;
                    usleep(100000); /* 100ms */
                }
                else
                {
                    ret = 0; /* ok */
                    goto leave;
                }
            }
        }
        usleep(100000); /* 100ms */
        chan_start(idx);
        retry_times = ch->retry_times;
    }

leave:
    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, fail",
                __func__, idx, slaveaddr, regaddr, nb);
    }
    return ret;
}

// FUNC CODE: 0x04
int chan_read_input_registers(int idx, int slaveaddr, int regaddr, int nb, unsigned short *dest)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_read_input_registers(ch->ctx, regaddr, nb, dest);
        modbus_flush(ch->ctx);
        if (rc != nb)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, rc:%d",
                __func__,
                idx,
                slaveaddr,
                regaddr,
                nb,
                rc);
    }
    return ret;
}

int chan_read_input_registers_with_retry(int idx, int slaveaddr, int regaddr, int nb, unsigned short *regval)
{
    int rc;
    int ret = -1;
    struct chan_t *ch = &chan[idx];
    int retry_times = ch->retry_times;
    int restart_times = ch->restart_times;

    while (restart_times-- > 0)
    {
        while (retry_times-- > 0)
        {
            if (ch->ctx != NULL)
            {
                modbus_set_slave(ch->ctx, slaveaddr);
                rc = modbus_read_input_registers(ch->ctx, regaddr, nb, regval);
                modbus_flush(ch->ctx);
                if (rc != nb)
                {
                    ch->errcnt++;
                    usleep(100000); /* 100ms */
                }
                else
                {
                    ret = 0; /* ok */
                    goto leave;
                }
            }
        }
        usleep(100000); /* 100ms */
        chan_start(idx);
        retry_times = ch->retry_times;
    }

leave:
    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, fail",
                __func__, idx, slaveaddr, regaddr, nb);
    }
    return ret;
}
// func code:0x01
// nb : bits count
int chan_read_bits(int idx, int slaveaddr, int regaddr, int nb, unsigned char *dest)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_read_bits(ch->ctx, regaddr, nb, dest);
        modbus_flush(ch->ctx);
        if (rc != nb)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, rc:%d",
                __func__,
                idx,
                slaveaddr,
                regaddr,
                nb,
                rc);
    }
    return ret;
}

// func code:0x05
// nb : bits count
int chan_write_bit(int idx, int slaveaddr, int addr, int status)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_write_bit(ch->ctx, addr, status);
        modbus_flush(ch->ctx);
        if (rc <= 0)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, status:%d, ret:%d",
                __func__,
                idx,
                slaveaddr,
                addr,
                status,
                ret);
    }
    return ret;
}

int chan_write_bits(int idx, int slaveaddr, int addr, int nb, unsigned char *status)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_write_bits(ch->ctx, addr, nb, status);
        modbus_flush(ch->ctx);
        if (rc <= 0)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, status:%hhn, ret:%d",
                __func__,
                idx,
                slaveaddr,
                addr,
                status,
                ret);
    }
    return ret;
}

int chan_read_gpio(int idx, int *val)
{
    chan_lock(idx);
    // log_dbg("read gpio, chan idx %d\n", idx);
    char buf[4096] = {0};
    sprintf(buf, "/sys/class/gpio/gpio%s/value", chan[idx].szdev);
    int fd_value = -1;

    if (chan[idx].mode != CHANMODE_DO && chan[idx].mode != CHANMODE_DI)
    {
        log_dbg("channel %d is not DIDO mode\n", idx);
        chan_unlock(idx);
        return -1;
    }

    fd_value = open(buf, O_RDONLY);
    if (fd_value == -1)
    {
        log_dbg("read value  %s\n", strerror(errno));
        chan_unlock(idx);
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    lseek(fd_value, 0, SEEK_SET);
    int rdsize = read(fd_value, buf, sizeof(buf));
    // log_dbg("read value size : %d\n", rdsize);
    if (rdsize < 0)
    {
        log_dbg("read value %s\n", strerror(errno));
        close(fd_value);
        chan_unlock(idx);
        return -1;
    }
    // log_dbg("gpio%s value: %s\n", chan[idx].szdev, buf);
    close(fd_value);
    *val = atoi(buf);
    chan_unlock(idx);
    return 0;
}

int chan_write_gpio(int idx, int val)
{
    chan_lock(idx);
    // log_dbg("write gpio, chan idx %d\n", idx);
    char buf[4096] = {0};
    sprintf(buf, "/sys/class/gpio/gpio%s/value", chan[idx].szdev);
    int fd_value = -1;
    if (chan[idx].mode != CHANMODE_DO)
    {
        log_dbg("channel %d is not DO mode\n", idx);
        chan_unlock(idx);
        return -1;
    }
    else
    {
        fd_value = open(buf, O_RDWR);
        if (fd_value == -1)
        {
            log_dbg("value  %s\n", strerror(errno));
            chan_unlock(idx);
            return -1;
        }
        sprintf(buf, "%d", val);
        if (write(fd_value, buf, sizeof(buf)) == -1)
        {
            log_dbg("write value %s\n", strerror(errno));
            close(fd_value);
            chan_unlock(idx);
            return -1;
        }
        // log_dbg("write value %s success\n", buf);
    }
    close(fd_value);
    chan_unlock(idx);
    return 0;
}

int chan_read_holdingregisters(int idx, int slaveaddr, int regaddr, int nb, unsigned short *regval)
{
    int rc;
    int ret = 0;
    struct chan_t *ch = &chan[idx];

    if (ch->ctx == NULL)
    {
        ret = -1;
    }
    else
    {
        modbus_set_slave(ch->ctx, slaveaddr);
        rc = modbus_read_registers(ch->ctx, regaddr, nb, regval);
        modbus_flush(ch->ctx);
        if (rc != nb)
        {
            ch->errcnt++;
            ret = -2;
        }
    }

    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, rc:%d",
                __func__, idx, slaveaddr, regaddr, nb, rc);
    }
    return ret;
}

int chan_read_holdingregisters_with_retry(int idx, int slaveaddr, int regaddr, int nb, unsigned short *regval)
{
    int rc;
    int ret = -1;
    struct chan_t *ch = &chan[idx];
    int retry_times = ch->retry_times;
    int restart_times = ch->restart_times;

    while (restart_times-- > 0)
    {
        while (retry_times-- > 0)
        {
            if (ch->ctx != NULL)
            {
                modbus_set_slave(ch->ctx, slaveaddr);
                rc = modbus_read_registers(ch->ctx, regaddr, nb, regval);
                modbus_flush(ch->ctx);
                if (rc != nb)
                {
                    ch->errcnt++;
                    usleep(100000); /* 100ms */
                }
                else
                {
                    ret = 0; /* ok */
                    goto leave;
                }
            }
        }
        usleep(100000); /* 100ms */
        chan_start(idx);
        retry_times = ch->retry_times;
    }

leave:
    if (ret < 0)
    {
        log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, nb:%d, fail",
                __func__, idx, slaveaddr, regaddr, nb);
    }
    return ret;
}

int chan_enable_dbg(int idx)
{
    int ret = 0;

    struct chan_t *ch = &chan[idx];
    if (ch->ctx != NULL)
    {
        ch->dbg = 1;
        modbus_set_debug(ch->ctx, 1);
    }
    else
    {
        ch->dbg = 1;
    }
    return ret;
}

int chan_disable_dbg(int idx)
{
    int ret = 0;

    struct chan_t *ch = &chan[idx];
    if (ch->ctx != NULL)
    {
        ch->dbg = 0;
        modbus_set_debug(ch->ctx, 0);
    }
    else
    {
        ch->dbg = 0;
    }
    return ret;
}

int chan_set_dbg(int idx, int enable)
{
    int ret = 0;
    struct chan_t *ch = &chan[idx];
    ch->dbg = enable;
    if (ch->ctx != NULL)
    {
        modbus_set_debug(ch->ctx, enable);
    }
    return ret;
}

int chan_set_en(int idx, int enable)
{
    int ret = 0;
    struct chan_t *ch = &chan[idx];
    ch->en = enable;

    log_dbg("%s, idx:%d, enable:%d", __func__, idx, enable);
    return ret;
}

int chan_get_en(int idx)
{

    struct chan_t *ch = &chan[idx];
    return ch->en;
}

void chan_serial_rxrb_init(int idx)
{
    chan_serial_ringbuffer_init(chan[idx].serial_rxrb);
}

void chan_serial_txrb_init(int idx)
{
    chan_serial_ringbuffer_init(chan[idx].serial_txrb);
}

void chan_serial_rxrb_queue(int idx, chan_serial_ringbuffer_element_t e)
{
    chan_serial_ringbuffer_queue(chan[idx].serial_rxrb, e);
}

int chan_serial_rxrb_dequeue_arr(int idx, chan_serial_ringbuffer_element_t *e, int len)
{
    return chan_serial_ringbuffer_dequeue_arr(chan[idx].serial_rxrb, e, len);
}

int chan_serial_rxrb_num_items(int idx)
{
    return chan_serial_ringbuffer_num_items(chan[idx].serial_rxrb);
}

int chan_serial_txrb_num_items(int idx)
{
    return chan_serial_ringbuffer_num_items(chan[idx].serial_txrb);
}

int chan_serial_txrb_dequeue_arr(int idx, chan_serial_ringbuffer_element_t *e, int len)
{
    return chan_serial_ringbuffer_dequeue_arr(chan[idx].serial_txrb, e, len);
}

void chan_serial_txrb_queue_arr(int idx, chan_serial_ringbuffer_element_t *e, int len)
{
    chan_serial_ringbuffer_queue_arr(chan[idx].serial_txrb, e, len);
}

void chan_socketcan_rxrb_init(int idx)
{
    chan_socketcan_ringbuffer_init(chan[idx].socketcan_rxrb);
}

void chan_socketcan_txrb_init(int idx)
{
    chan_socketcan_ringbuffer_init(chan[idx].socketcan_txrb);
}

void chan_socketcan_rxrb_queue(int idx, chan_socketcan_ringbuffer_element_t e)
{
    chan_socketcan_ringbuffer_queue(chan[idx].socketcan_rxrb, e);
}

void chan_socketcan_rxrb_queue_arr(int idx, chan_socketcan_ringbuffer_element_t *e, int len)
{
    chan_socketcan_ringbuffer_queue_arr(chan[idx].socketcan_rxrb, e, len);
}

int chan_socketcan_rxrb_dequeue(int idx, chan_socketcan_ringbuffer_element_t *e)
{
    return chan_socketcan_ringbuffer_dequeue(chan[idx].socketcan_rxrb, e);
}

int chan_socketcan_rxrb_dequeue_arr(int idx, chan_socketcan_ringbuffer_element_t *e, int len)
{
    return chan_socketcan_ringbuffer_dequeue_arr(chan[idx].socketcan_rxrb, e, len);
}

int chan_socketcan_txrb_num_items(int idx)
{
    return chan_socketcan_ringbuffer_num_items(chan[idx].socketcan_txrb);
}

int chan_socketcan_rxrb_num_items(int idx)
{
    return chan_socketcan_ringbuffer_num_items(chan[idx].socketcan_rxrb);
}

int chan_socketcan_txrb_dequeue_arr(int idx, chan_socketcan_ringbuffer_element_t *e, int len)
{
    return chan_socketcan_ringbuffer_dequeue_arr(chan[idx].socketcan_txrb, e, len);
}

int chan_socketcan_txrb_dequeue(int idx, chan_socketcan_ringbuffer_element_t *e)
{
    return chan_socketcan_ringbuffer_dequeue(chan[idx].socketcan_txrb, e);
}

void chan_socketcan_txrb_queue(int idx, chan_socketcan_ringbuffer_element_t e)
{
    return chan_socketcan_ringbuffer_queue(chan[idx].socketcan_txrb, e);
}

void chan_tcpservcan_rxrb_init(int idx)
{
    chan_tcpservcan_ringbuffer_init(chan[idx].tcpservcan_rxrb);
}

void chan_tcpservcan_txrb_init(int idx)
{
    chan_tcpservcan_ringbuffer_init(chan[idx].tcpservcan_txrb);
}

void chan_tcpservcan_rxrb_queue(int idx, chan_tcpservcan_ringbuffer_element_t e)
{
    chan_tcpservcan_ringbuffer_queue(chan[idx].tcpservcan_rxrb, e);
}

int chan_tcpservcan_txrb_num_items(int idx)
{
    return chan_tcpservcan_ringbuffer_num_items(chan[idx].tcpservcan_txrb);
}

int chan_tcpservcan_txrb_dequeue_arr(int idx, chan_tcpservcan_ringbuffer_element_t *e, int len)
{
    return chan_tcpservcan_ringbuffer_dequeue_arr(chan[idx].tcpservcan_txrb, e, len);
}

int chan_start(int idx)
{
    pthread_t xthrd;
    struct hostent *he;
    struct sockaddr_in my_addr;
    struct timeval t;
    int sock;
    struct sockaddr_can addr;
    int ret = 0;
    int rc = 0;
    struct chan_t *ch = NULL;
    char buf[128];
    struct timeval timeout = {1, 0};
    int recv_own_msgs = 0; // set loop back:  1 enable 0 disable
    const UCHAR ucSlaveID[] = {0xAA, 0xBB, 0xCC};
    struct sockaddr_in servaddr;
    int slaveadr = 0;

    log_dbg("%s, ++, idx:%d", __func__, idx);

    ch = &chan[idx];
    ch->started = 0;

    if (ch->mode == CHANMODE_SERIAL)
    {
        log_dbg("%s, idx:%d, mode:%s, dev:%s, parity:%s, baud:%d", __func__, idx, ch->szmode, ch->szdev, ch->szparity, ch->baud);
        sprintf(buf, "/dev/%s", ch->szdev);
        // ch->fd = open(buf, O_RDWR);
        ch->fd = chan_open_serial(buf, ch->baud, 8, ch->szparity[0], 1);
        if (ch->fd < 0)
        {
            log_dbg("%s, idx:%d, open %s fail", __func__, idx, buf);
            ch->fd = 0;
            ret = -1;
        }
        else
        {
            // if(chan_set_baud(ch->fd,ch->baud) < 0){
            //     log_dbg("%s, idx:%d, set baud %s %d fail", __func__, idx, buf,ch->baud);
            //     close(ch->fd);
            //     ch->fd = 0;
            // }else if( chan_set_parity(ch->fd, 8, 1, ch->szparity[0]) < 0 ){
            //     log_dbg("%s, idx:%d, set parity %s %s fail", __func__, idx, buf, ch->szparity);
            //     close(ch->fd);
            //     ch->fd = 0;
            //     ret = -1;
            // }else{
            ch->serial_rxrb = (chan_serial_ringbuffer_t *)malloc(sizeof(chan_serial_ringbuffer_t));
            if (ch->serial_rxrb != NULL)
            {
                chan_serial_rxrb_init(idx);
                ch->serial_txrb = (chan_serial_ringbuffer_t *)malloc(sizeof(chan_serial_ringbuffer_t));
                if (ch->serial_txrb != NULL)
                {
                    chan_serial_txrb_init(idx);
                }
                else
                {
                    log_dbg("%s, idx:%d, malloc tx ringbuffer fail", __func__, idx);
                    free(ch->serial_rxrb);
                    ret = -1;
                }
            }
            else
            {
                log_dbg("%s, idx:%d, malloc rx ringbuffer fail", __func__, idx);
                ret = -1;
            }
            chanthrd_param[idx].idx = idx;
            if (pthread_create(&xthrd, NULL, chan_serial_rxthrd, &chanthrd_param[idx]) != 0)
            {
                log_dbg("%s, chan_serial_rxthrd create fail", __func__);
                ret = -1;
            }
            else if (pthread_create(&xthrd, NULL, chan_serial_txthrd, &chanthrd_param[idx]) != 0)
            {
                log_dbg("%s, chan_serial_rxthrd create fail", __func__);
                ret = -1;
            }
            //}
        }
    }
    else if (ch->mode == CHANMODE_SOCKET_CAN)
    {
        log_dbg("%s, idx:%d, mode:%s, dev:%s", __func__, idx, ch->szmode, ch->szdev);
        sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (sock < 0)
        {
            printf("error\n");
            ret = -1;
        }
        else
        {
            addr.can_family = AF_CAN;
            strcpy(ifr[idx].ifr_name, ch->szdev);
            rc = ioctl(sock, SIOCGIFINDEX, &ifr[idx]); // get index
            if (rc && ifr[idx].ifr_ifindex == 0)
            {
                log_dbg("%s, idx:%d, rc && ifr[idx].ifr_ifindex == 0", __func__, idx);
                close(sock);
                ret = -1;
            }
            else
            {
                addr.can_ifindex = ifr[idx].ifr_ifindex;
                setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));
                if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
                {
                    log_dbg("%s, idx:%d, bind error", __func__, idx);
                    close(sock);
                    ret = -1;
                }
                else
                {
                    ch->sockfd = sock;
                    ch->socketcan_rxrb = (chan_socketcan_ringbuffer_t *)malloc(sizeof(chan_socketcan_ringbuffer_t));
                    if (ch->socketcan_rxrb != NULL)
                    {
                        chan_socketcan_rxrb_init(idx);
                        ch->socketcan_txrb = (chan_socketcan_ringbuffer_t *)malloc(sizeof(chan_socketcan_ringbuffer_t));
                        if (ch->socketcan_txrb != NULL)
                        {
                            chan_socketcan_txrb_init(idx);
                        }
                        else
                        {
                            log_dbg("%s, idx:%d, malloc tx ringbuffer fail", __func__, idx);
                            free(ch->socketcan_rxrb);
                            ret = -1;
                        }
                    }
                    else
                    {
                        log_dbg("%s, idx:%d, malloc rx ringbuffer fail", __func__, idx);
                        ret = -1;
                    }
                    chanthrd_param[idx].idx = idx;
                    if (pthread_create(&xthrd, NULL, chan_socketcan_rxthrd, &chanthrd_param[idx]) != 0)
                    {
                        log_dbg("%s, chan_socketcan_rxthrd create fail", __func__);
                        ret = -1;
                    }
                    else if (pthread_create(&xthrd, NULL, chan_socketcan_txthrd, &chanthrd_param[idx]) != 0)
                    {
                        log_dbg("%s, chan_socketcan_rxthrd create fail", __func__);
                        ret = -1;
                    }
                }
            }
        }
    }
    else if (ch->mode == CHANMODE_TCPSERV_CAN)
    {
        log_dbg("%s, idx:%d, mode:%s, host:%s, port:%d", __func__, idx, ch->szmode, ch->servip, ch->servport);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
            log_dbg("%s, tcpserv_can socket fail", __func__);
            ret = -1;
        }
        else
        {
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(ch->servport);
            inet_pton(AF_INET, ch->servip, &servaddr.sin_addr);
            rc = connect(sock, (SA *)&servaddr, sizeof(servaddr));
            if (rc < 0)
            {
                log_dbg("%s, tcpserv_can connect fail", __func__);
                ret = -1;
            }
            else
            {
                ch->sockfd = sock;
                ch->tcpservcan_rxrb = (chan_tcpservcan_ringbuffer_t *)malloc(sizeof(chan_tcpservcan_ringbuffer_t));
                if (ch->tcpservcan_rxrb != NULL)
                {
                    chan_tcpservcan_rxrb_init(idx);
                    ch->tcpservcan_txrb = (chan_tcpservcan_ringbuffer_t *)malloc(sizeof(chan_tcpservcan_ringbuffer_t));
                    if (ch->tcpservcan_txrb != NULL)
                    {
                        chan_tcpservcan_txrb_init(idx);
                    }
                    else
                    {
                        log_dbg("%s, idx:%d, malloc tx ringbuffer fail", __func__, idx);
                        free(ch->tcpservcan_rxrb);
                        ret = -1;
                    }
                }
                else
                {
                    log_dbg("%s, idx:%d, malloc rx ringbuffer fail", __func__, idx);
                    ret = -1;
                }
                chanthrd_param[idx].idx = idx;
                if (pthread_create(&xthrd, NULL, chan_tcpservcan_rxthrd, &chanthrd_param[idx]) != 0)
                {
                    log_dbg("%s, chan_tcpservcan_rxthrd create fail", __func__);
                    ret = -1;
                }
                else if (pthread_create(&xthrd, NULL, chan_tcpservcan_txthrd, &chanthrd_param[idx]) != 0)
                {
                    log_dbg("%s, chan_tcpservcan_rxthrd create fail", __func__);
                    ret = -1;
                }
            }
        }
    }
    else if (ch->mode == CHANMODE_MODBUS_RTU)
    {
        sprintf(buf, "/dev/%s", ch->szdev);
        log_dbg("%s, idx:%d, mode:%s, dev:%s", __func__, idx, ch->szmode, buf);
        if (ch->ctx != NULL)
        {
            log_dbg("%s, idx:%d, modbus rtu, ctx:0x%08X, cleared", __func__, idx, (unsigned int)(ch->ctx));
            modbus_close(ch->ctx);
            modbus_free(ch->ctx);
            ch->ctx = NULL;
        }
        ch->ctx = modbus_new_rtu(buf, ch->baud, ch->szparity[0], 8, 1);
        if (ch->ctx == NULL)
        {
            modbus_set_debug(ch->ctx, 1);
            log_dbg("%s, idx:%d, modbus rtu new fail", __func__, idx);
            ch->started = 0;
        }
        else if (modbus_connect(ch->ctx) == -1)
        {
            log_dbg("%s, idx:%d, modbus rtu connect fail", __func__, idx);
            modbus_free(ch->ctx);
            ch->ctx = NULL;
        }
        else
        {
            t.tv_sec = 0;
            t.tv_usec = 1000000; // 20s, 为了减少一些慢速设备超时导致的大量的日志
            modbus_set_response_timeout(ch->ctx, &t);
        }
    }
    else if (strcmp(ch->szmode, "modbus_rtu_udp") == 0)
    {
        // sprintf(buf,"/dev/%s",chn->szPort);
        ch->ctx = modbus_new_rtuudp(ch->servip, ch->servport);
        if (ch->ctx == NULL)
        {
            ret = -3;
        }
        else if (modbus_connect(ch->ctx) == -1)
        {
            ret = -4;
            modbus_free(ch->ctx);
        }
        else
        {
            t.tv_sec = 0;
            t.tv_usec = 1000000; // 1000ms
            modbus_set_response_timeout(ch->ctx, &t);
        }
    }
    else if (strcmp(ch->szmode, "modbus_rtu_tcp") == 0)
    {
        // sprintf(buf,"/dev/%s",chn->szPort);
        ch->ctx = modbus_new_rtutcp(ch->servip, ch->servport);
        if (ch->ctx == NULL)
        {
            ret = -5;
        }
        else if (modbus_connect(ch->ctx) == -1)
        {
            ret = -6;
            modbus_free(ch->ctx);
        }
        else
        {
            t.tv_sec = 0;
            t.tv_usec = 1000000; // 1000ms
            modbus_set_response_timeout(ch->ctx, &t);
        }
    }
    else if (ch->mode == CHANMODE_MODBUS_TCP)
    {
        if (ch->ctx != NULL)
        {
            log_dbg("%s, idx:%d, modbus tcp, ctx:0x%08X, cleared", __func__, idx, (unsigned int)(ch->ctx));
            modbus_close(ch->ctx);
            modbus_free(ch->ctx);
            ch->ctx = NULL;
        }
        ch->ctx = modbus_new_tcp(ch->servip, ch->servport);
        if (ch->ctx == NULL)
        {
            log_dbg("%s, idx:%d, modbus tcp new fail", __func__, idx);
            ch->started = 0;
        }
        else if (modbus_connect(ch->ctx) == -1)
        {
            log_dbg("%s, idx:%d, modbus tcp connect fail", __func__, idx);
            modbus_free(ch->ctx);
            ch->ctx = NULL;
            ch->started = 0;
        }
        else
        {
            log_dbg("%s, idx:%d, modbus tcp, ctx:0x%08X, start ok", __func__, idx, (unsigned int)(ch->ctx));
            ch->started = 1;
        }
    }
    else if (ch->mode == CHANMODE_DI || ch->mode == CHANMODE_DO)
    {
        char buf[4096] = {0};
        struct stat file_info;
        sprintf(buf, "/sys/class/gpio/gpio%s", ch->szdev);
        if (stat(buf, &file_info) != -1)
        {
            log_dbg("stat : gpio path exported: %s\n", buf);
            // if (file_info.st_mode & S_IFDIR)
            // {
            //     log_dbg("%s, gpio%s has been exported\n", ch->szdev);
            // }
        }
        else
        {
            log_dbg("%s, exporting gpio%s \n", __func__, ch->szdev);
            int fd_export = open(EXPORT_PATH, O_WRONLY);
            if (fd_export == -1)
            {
                log_dbg("open path %s failed : %s\n", EXPORT_PATH, strerror(errno));
                return -1;
            }
            if (write(fd_export, ch->szdev, strlen(ch->szdev)) == -1)
            {
                log_dbg("write path %s\n", strerror(errno));
                close(fd_export);
                return -1;
            }
            log_dbg("export / unexport %s success\n", ch->szdev);
            close(fd_export);
        }
        // 设置gpio方向
        log_dbg("set gpio%s direction\n", ch->szdev);
        sprintf(buf, "/sys/class/gpio/gpio%s/direction", ch->szdev);
        log_dbg("direction path: %s\n", buf);
        int fd_direction = open(buf, O_RDWR);
        if (fd_direction == -1)
        {
            log_dbg("direction  %s\n", strerror(errno));
            return -1;
        }
        if (ch->mode == CHANMODE_DI)
        {
            sprintf(buf, "%s", DIR_IN);
        }
        else
        {
            sprintf(buf, "%s", DIR_OUT);
        }
        if (write(fd_direction, buf, strlen(buf)) == -1)
        {
            log_dbg("write direction %s\n", strerror(errno));
            close(fd_direction);
            return -1;
        }
        log_dbg("set gpio%s direction success\n", ch->szdev);
        close(fd_direction);
        ret = 0;
    }
    else
    {
        log_dbg("%s, unknown ch mode :%s", __func__, ch->szmode);
        ret = -1;
    }

leave:
    log_dbg("%s, --,idx:%d, ret:%d", __func__, idx, ret);
    return ret;
}

int chan_mode_str2nbr(char *szmode)
{
    int mode = CHANMODE_UNKNOWN;

    if (szmode == NULL)
    {
        return CHANMODE_UNKNOWN;
    }
    if (strcmp(szmode, "serial") == 0)
    {
        mode = CHANMODE_SERIAL;
    }
    else if (strcmp(szmode, "socket_can") == 0)
    {
        mode = CHANMODE_SOCKET_CAN;
    }
    else if (strcmp(szmode, "tcp_can") == 0)
    {
        mode = CHANMODE_TCPSERV_CAN;
    }
    else if (strcmp(szmode, "modbus_rtu") == 0)
    {
        mode = CHANMODE_MODBUS_RTU;
    }
    else if (strcmp(szmode, "modbus_tcp") == 0)
    {
        mode = CHANMODE_MODBUS_TCP;
    }
    else if (strcmp(szmode, "di") == 0)
    {
        mode = CHANMODE_DI;
    }
    else if (strcmp(szmode, "do") == 0)
    {
        mode = CHANMODE_DO;
    }
    else
    {
        mode = CHANMODE_UNKNOWN;
    }

    return mode;
}

int chan_reset(int idx)
{
    struct timeval t;
    int ret = 0;
    char buf[32];
    struct chan_t *ch = &chan[idx];

    log_dbg("%s, ++, idx:%d", __func__, idx);

    if (strcmp(ch->szmode, "modbus_tcp") == 0)
    {
        ch->rstcnt += 1;
        if (ch->ctx != NULL)
        {
            modbus_close(ch->ctx);
            modbus_free(ch->ctx);
        }
        ch->ctx = modbus_new_tcp(ch->servip, ch->servport);
        if (ch->ctx == NULL)
        {
            ret = -1;
        }
        else if (modbus_connect(ch->ctx) == -1)
        {
            modbus_free(ch->ctx);
            ch->ctx = NULL;
            ret = -2;
        }
        else
        {
            log_dbg("%s, idx:%d, ctx:0x%08X init success", __func__, idx, (unsigned int)(ch->ctx));
        }
    }
    else
    {
        ret = -10;
    }

    log_dbg("%s, --, idx:%d, ret:%d", __func__, idx, ret);
    return ret;
}

int chan_get_mode(int idx)
{
    return chan[idx].mode;
}

char *chan_get_info_str(int idx)
{
    if (idx > chan_get_nbr())
    {
        return NULL;
    }
    else
    {
        return chan[idx].szinfo;
    }
}

char *chan_get_servip_str(int idx)
{
    if (idx > chan_get_nbr())
    {
        return NULL;
    }
    else
    {
        return chan[idx].servip;
    }
}

int chan_get_servport(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].servport;
    }
}

int chan_get_baud(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].baud;
    }
}

char *chan_get_parity_str(int idx)
{
    if (idx > chan_get_nbr())
    {
        return NULL;
    }
    else
    {
        return chan[idx].szparity;
    }
}

char *chan_get_dev_str(int idx)
{
    if (idx > chan_get_nbr())
    {
        return NULL;
    }
    else
    {
        return chan[idx].szdev;
    }
}

int chan_get_port(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].port;
    }
}

char *chan_get_mode_str(int idx)
{
    if (idx > chan_get_nbr())
    {
        return NULL;
    }
    else
    {
        return chan[idx].szmode;
    }
}

int chan_get_errcnt(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].errcnt;
    }
}

int chan_get_started(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].started;
    }
}

int chan_get_restart_times(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].restart_times;
    }
}

int chan_get_retry_times(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].retry_times;
    }
}

int chan_get_dbg(int idx)
{
    if (idx > chan_get_nbr())
    {
        return -1;
    }
    else
    {
        return chan[idx].dbg;
    }
}

int chan_get_tool_data(int idx, char *buf)
{
    struct chan_t *ch = &chan[idx];
    sprintf(buf, "  [%d]info:%s mode:%s dev:%s port:%d servip:%s servport:%d baud:%d dbg:%d started:%d errcnt:0x%08X retry_times:%d restart_times:%d\n",
            idx, ch->szinfo, ch->szmode, ch->szdev, ch->port, ch->servip, ch->servport,
            ch->baud, ch->dbg, ch->started, ch->errcnt, ch->retry_times, ch->restart_times);
}

int chan_get_tool_all_data(char *buf)
{
    int i = 0;
    char buf_temp[1024];
    sprintf(buf, "" REVERSE " CHAN " NONE "  nbr:%d\n", channbr);
    for (i = 1; i <= chan_get_nbr(); i++)
    {
        chan_get_tool_data(i, buf_temp);
        // printf("%s",buf_temp);
        strcat(buf, buf_temp);
    }
    return 0;
}

int chan_get_bkds_data(char *buf)
{
    char buf_temp[4096];
    int i = 0;

    sprintf(buf, "\"chan\":[");
    for (i = 1; i <= chan_get_nbr(); i++)
    {
        memset(buf_temp, 0, sizeof(buf_temp));
        sprintf(buf_temp, "{\"info\":\"%s\",\"servip\":\"%s\",\"servport\":%d,\"baud\":%d,\"parity\":\"%s\",\
\"dev\":\"%s\",\"port\":%d,\"mode\":\"%s\",\"errcnt\":%d,\"started\":%d,\
\"restart_times\":%d,\"retry_times\":%d,\"dbg\":%d}",
                chan_get_info_str(i), chan_get_servip_str(i), chan_get_servport(i), chan_get_baud(i), chan_get_parity_str(i),
                chan_get_dev_str(i), chan_get_port(i), chan_get_mode_str(i), chan_get_errcnt(i), chan_get_started(i),
                chan_get_restart_times(i), chan_get_retry_times(i), chan_get_dbg(i));

        strcat(buf, buf_temp);
        if (i < chan_get_nbr())
        {
            strcat(buf, ",");
        }
    };
    strcat(buf, "]");
    return 0;
}
