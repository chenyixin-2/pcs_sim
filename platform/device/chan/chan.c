#include "plt.h"
#include "chan_ringbuffer.h"
#include "chan.h"
#include "log.h"
#include "mb.h"
#include "port.h"
#include "mbctx.h"

#define	SA	struct sockaddr
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

struct chanthrd_param_t
{
    int bidx;
    int idx;
    int ival;
};
static struct chanthrd_param_t chanthrd_param[CHAN_NBR_MAX + 1];
/* rx buffer */
static chan_ring_buffer_t _chan_rx_rb[CHAN_NBR_MAX + 1];
/* tx buffer */
static chan_ring_buffer_t _chan_tx_rb[CHAN_NBR_MAX + 1];

static void chan_dump(char* buf_dst, unsigned char* buf_src, int len)
{
    char buf_t[16];
    int i;

    buf_dst[0] = 0;
    for(i = 0; i<len; i++){
        sprintf(buf_t,"%02X ",buf_src[i]);
        strcat(buf_dst,buf_t);
    }
}

static int chan_set_baud(int fd, int speed)
{
    int   i;
    int   status;
    struct termios   Opt;
    int speed_arr[] = {  B115200, B57600, B38400, B19200, B9600, B4800,	B2400, B1200};
    int name_arr[] = {115200, 57600, 38400,  19200,  9600,  4800,  2400, 1200};

    tcgetattr(fd, &Opt);
    for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++){
        if(speed == name_arr[i]){
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&Opt, speed_arr[i]);
            cfsetospeed(&Opt, speed_arr[i]);
            status = tcsetattr(fd, TCSANOW, &Opt);
            if (status != 0){
                perror("tcsetattr fail\n");
                return -1;
            }else{
                return 0;
            }
        }
        tcflush(fd,TCIOFLUSH);
    }
    return -1;
}

static int chan_set_parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if( tcgetattr( fd,&options)  !=  0){
        perror("SetupSerial 1");
        return -1;
    }
    options.c_cflag &= ~CSIZE;
    switch(databits){
        case 7:	options.c_cflag |= CS7;
            break;
        case 8:	options.c_cflag |= CS8;
            break;
        default:
            fprintf(stderr,"Unsupported data size\n");
            return -1;
    }
    switch(parity){
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
            fprintf(stderr,"Unsupported parity\n");
            return -1;
    }
    switch(stopbits){
        case 1:
            options.c_cflag &= ~CSTOPB;
            break;
        case 2:
            options.c_cflag |= CSTOPB;
            break;
        default:
            fprintf(stderr,"Unsupported stop bits\n");
            return -1;
    }
    options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
    options.c_oflag &= ~OPOST;
    options.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    /* Set input parity option */
    if (parity != 'n')
        options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 150; // 15 seconds
    options.c_cc[VMIN] = 0;
    tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)    {
        perror("SetupSerial 3");
        return -1;
    }
    return 0;
}

#define PF_CAN 29
#define AF_CAN PF_CAN
#define SIOCSCANBAUDRATE        (SIOCDEVPRIVATE+0)
#define SIOCGCANBAUDRATE        (SIOCDEVPRIVATE+1)
#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)
#define CAN_RAW_FILTER  1
#define CAN_RAW_RECV_OWN_MSGS 0x4
typedef __u32 can_baudrate_t;
struct ifreq ifr[CHAN_NBR_MAX + 1];

static struct chanthrd_param_t s_chanthrd_param[CHAN_NBR_MAX + 1];

static int chan_dbcb_0(void *para,int ncolumn,char ** columnvalue,char *columnname[])
{
    int i;
    struct dbcbparam_t* pcbparam = (struct dbcbparam_t*)para;
    struct chan_t* pchan;

    pcbparam->nrow++;
    log_dbg("%s, ++,row:%d;col:%d",__func__,pcbparam->nrow,ncolumn);

    pchan = &mdl.chan[pcbparam->nrow];
    for( i = 0; i < ncolumn; i++){
        if( strcmp("info",columnname[i]) == 0){
            strcpy(pchan->szinfo,columnvalue[i]);
        }else if( strcmp("dev",columnname[i]) == 0){
            strcpy(pchan->szdev,columnvalue[i]);
        }else if( strcmp("mode",columnname[i]) == 0){
            strcpy(pchan->szmode,columnvalue[i]);
            pchan->mode=chan_mode_str2nbr(pchan->szmode);
        }else if( strcmp("port",columnname[i]) == 0){
            pchan->port = atoi(columnvalue[i]);
        }else if( strcmp("baud",columnname[i]) == 0){
            pchan->baud = atoi(columnvalue[i]);
        }else if( strcmp("parity",columnname[i]) == 0){
            strcpy(pchan->szparity, columnvalue[i]);
        }else if( strcmp("servip",columnname[i]) == 0){
            strcpy(pchan->servip,columnvalue[i]);
        }else if( strcmp("servport",columnname[i]) == 0){
            pchan->servport = atoi(columnvalue[i]);
        }else if( strcmp("mbsidx",columnname[i]) == 0){
            pchan->mbsidx = atoi(columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, --,ret:%d",__func__,pcbparam->ret);
    return 0;
}

int chan_mode_str2nbr(char* szmode)
{
    int mode = CHANMODE_UNKNOWN;

    if( szmode == NULL ){
        return CHANMODE_UNKNOWN;
    }

    if( strcmp(szmode, "serial") == 0){
        mode = CHANMODE_SERIAL;
    }else if( strcmp(szmode, "socket_can") == 0){
        mode = CHANMODE_SOCKET_CAN;
    }else if( strcmp(szmode, "tcp_can") == 0){
        mode = CHANMODE_TCPSERV_CAN;
    }else if( strcmp(szmode, "modbus_rtu") == 0){
        mode = CHANMODE_MODBUS_RTU;
    }else if( strcmp(szmode, "modbus_rtu_over_tcp") == 0){
        mode = CHANMODE_MODBUS_RTU_OVER_TCP;
    }else if( strcmp(szmode, "modbus_rtu_over_udp") == 0){
        mode = CHANMODE_MODBUS_RTU_OVER_UDP;
    }else if( strcmp(szmode, "modbus_tcp") == 0){
        mode = CHANMODE_MODBUS_TCP;
    }else{
        mode = CHANMODE_UNKNOWN;
    }

    return mode;
}

static void _socketcan_readthrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    int i = 0;
    int rc = 0;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chanidx];
    int sock = chan->sockfd;
    int is_ext_frm = 0;
    unsigned int id = 0;
    ring_buffer_element_t e;
    char tmpbuf[256];

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while(1) {
        memset(&frame,0,sizeof(struct can_frame));
        rc = read(sock,&frame,sizeof(struct can_frame));
        //log_dbg("%s, read rc:%d, can_dlc:%d", __func__, rc, frame.can_dlc);
        if(frame.can_dlc){
            if( chan->dbg ){
                is_ext_frm = (frame.can_id & CAN_EFF_FLAG)?1:0;
                if(is_ext_frm){
                    id = frame.can_id & CAN_EFF_MASK;
                }else{
                    id = frame.can_id & CAN_SFF_MASK;
                }
                tmpbuf[0] = 0;
                chan_dump(tmpbuf, frame.data, frame.can_dlc);
                log_dbg("%s, ID:0x%08X, data:%s",__func__, id, tmpbuf);
            }

            chan_lock(chanidx);
            e.frame = frame;
            //chan_ring_buffer_queue(&ring_buffer[chanidx], e);
            chan_unlock(chanidx);
         }
     }
}

static void _tcpservcan_readthrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    int i = 0;
    int rc = 0;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chanidx];
    int sock = chan->sockfd;
    int is_ext_frm = 0;
    unsigned int id = 0;
    ring_buffer_element_t e;
    char tmpbuf[256];

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while(1) {
        memset(&frame,0,sizeof(struct can_frame));
        rc = read(sock,&frame,sizeof(struct can_frame));
        if(frame.can_dlc){
            if( chan->dbg ){
                is_ext_frm = (frame.can_id & CAN_EFF_FLAG)?1:0;
                if(is_ext_frm){
                    id = frame.can_id & CAN_EFF_MASK;
                }else{
                    id = frame.can_id & CAN_SFF_MASK;
                }
                tmpbuf[0] = 0;
                chan_dump(tmpbuf, frame.data, frame.can_dlc);
                log_dbg("%s, ID:0x%08X, data:%s",__func__, id, tmpbuf);
            }

            chan_lock(chanidx);
            e.frame = frame;
            //chan_ring_buffer_queue(&chan_ring_buffer[chanidx], e);
            chan_unlock(chanidx);
         }
     }
}

static void _tcpservcan_writethrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chanidx];
    int sock = chan->sockfd;
    ring_buffer_element_t e;

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while(1) {
        //cnt = 0;
        chan_lock(chanidx);
        if(chan_ring_buffer_dequeue(chan->ring_buffer_tx, &e)!=0){
            write(sock, &e.frame, sizeof(struct can_frame));
        }
        chan_unlock(chanidx);
        usleep(1000);
     }
}

static void _socketcan_writethrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    struct can_frame frame;
    int chanidx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chanidx];
    int sock = chan->sockfd;
    ring_buffer_element_t e;
    //int cnt = 0;

    log_dbg("%s, ++, idx:%d", __func__, chanidx);
    while(1) {
        //cnt = 0;
        chan_lock(chanidx);
        if(chan_ring_buffer_dequeue(chan->ring_buffer_tx, &e)!=0){
            write(sock, &e.frame, sizeof(struct can_frame));
            //usleep(100);
            //if(++cnt > 20){
            //    break;
            //}
        }
        chan_unlock(chanidx);
        usleep(1000);
     }
}

static void chan_serial_rxthrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    int chan_idx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chan_idx];
    int fd = chan->fd;
    chan_ring_buffer_element_t e;
    int nread;
    char buff[128];
    char dump_buf[1024];
    int i;

    log_dbg("%s, ++, idx:%d, serial", __func__, chan_idx);
    while(true){
        if((nread = read(fd,buff,100))>0){
            if(  chan->dbg > 0){
                chan_dump(dump_buf, buff, nread);
                log_dbg("%s, idx:%d, rx %d bytes:%s",__func__, chan_idx, nread, dump_buf);
            }
            chan_lock(chan_idx);
            for( i = 0; i < nread; i++){
                e.c = buff[i];
                chan_ring_buffer_queue(&_chan_rx_rb[chan_idx], e);
            }
            chan_unlock(chan_idx);
        }
        usleep(1000);
    }
}

static void chan_serial_txthrd(void* thrdparam)
{
    struct chanthrd_param_t* pparam = (struct chanthrd_param_t* )thrdparam;
    int chan_idx = pparam->idx;
    struct chan_t* chan = &mdl.chan[chan_idx];
    int fd = chan->fd;
    chan_ring_buffer_element_t e_arr[128];
    int towrite;
    int needtowrite;
    char buff[128];
    char dump_buf[1024];
    int i;

    log_dbg("%s, ++, idx:%d, serial", __func__, chan_idx);
    while(true){
        chan_lock(chan_idx);
        needtowrite = chan_ring_buffer_num_items(&_chan_tx_rb[chan_idx]);
        if( needtowrite > 0 ){
            towrite = needtowrite>sizeof(buff)?sizeof(buff):needtowrite;
            chan_ring_buffer_dequeue_arr(&_chan_tx_rb[chan_idx], e_arr, towrite);
        }else{
            towrite = 0;
        }
        chan_unlock(chan_idx);
        if(towrite > 0){
            for( i = 0; i < towrite; i++){
                buff[i] = e_arr[i].c;
            }
            write(fd, buff, towrite);
            if(  chan->dbg > 0){
                chan_dump(dump_buf, buff, towrite);
                log_dbg("%s, idx:%d, tx %d bytes:%s",__func__, chan_idx, towrite, dump_buf);
            }
        }
        usleep(2000);
    }
}

void chan_set_mbs( int idx, int devm, int devidx )
{
    mdl.chan[idx].mbsdevm = devm;
    mdl.chan[idx].mbsdevidx = devidx;
}

int chan_init( void )
{
    int result = 0;
    int ret = 0;
    int i = 0;
    int* channbr = &mdl.channbr;
    struct chan_t* chan ;
    char *errmsg = NULL;
    char sql[1024];
    struct dbcbparam_t cbparam;
    sqlite3* db = mdl.cfg_db;

    log_dbg("%s, ++",__func__);

    sprintf(sql,"select * from chan");
    cbparam.nrow = 0;
	result = sqlite3_exec(db,sql, chan_dbcb_0,(void*)&cbparam,&errmsg);
	if( result != SQLITE_OK ){
        ret = -1;
	 }else if( cbparam.ret != 0){
        ret = -2;
	 }else{
        *channbr = cbparam.nrow ;
        for( i = 1; i <= *channbr; i++){
            chan = &mdl.chan[i];
            chan->rstcnt = 0;
            chan->dbg = 0;
            if( chan_start(i) < 0 ){
                log_dbg("%s, idx:%d, start fail",__func__,i);
            }else{
                log_dbg("%s, idx:%d, start ok",__func__,i);
            }
        }
	 }

    log_dbg("%s--, ret:%d",__func__,ret);
    return ret;
}
// func code:0x06
int chan_write_single_register(int idx, int slaveaddr, int regaddr, int regval )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_write_register( chan->ctx, regaddr, regval);
        modbus_flush(chan->ctx);
        if( rc != 1){
            chan->errcnt++;
            ret = -3;
        }
    }

    //log_dbg("%s, idx:%d, slaveaddr:%d, regaddr:%d, rc:%d, regval:%d",
    //                            __func__,idx,slaveaddr,regaddr,rc,regval);
    return ret;
}
// func code:0x10
int chan_write_multi_registers( int idx, int slaveaddr, int regaddr, int nb, unsigned short* regval )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_write_registers( chan->ctx, regaddr, nb, regval);
        modbus_flush(chan->ctx);
        if( rc != nb){
            chan->errcnt++;
            ret = -2;
        }
    }

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
int chan_read_input_bits( int idx, int slaveaddr, int regaddr, int nb, unsigned char* dest )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_read_input_bits( chan->ctx, regaddr, nb, dest);
        modbus_flush(chan->ctx);
        if( rc != nb ){
            chan->errcnt++;
            ret = -2;
        }
    }

    if( ret < 0 ){
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


// FUNC CODE: 0x04
int chan_read_input_registers( int idx, int slaveaddr, int regaddr, int nb, unsigned short* dest )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_read_input_registers( chan->ctx, regaddr, nb, dest);
        modbus_flush(chan->ctx);
        if( rc != nb ){
            chan->errcnt++;
            ret = -2;
        }
    }

    if( ret < 0 ){
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
// func code:0x01
// nb : bits count
int chan_read_bits( int idx, int slaveaddr, int regaddr, int nb, unsigned char* dest )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_read_bits( chan->ctx, regaddr, nb, dest);
        modbus_flush(chan->ctx);
        if( rc != nb ){
            chan->errcnt++;
            ret = -2;
        }
    }

    if( ret < 0 ){
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
int chan_write_bit( int idx, int slaveaddr, int addr, int status )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_write_bit( chan->ctx, addr, status);
        modbus_flush(chan->ctx);
        if( rc <= 0 ){
            chan->errcnt++;
            ret = -2;
        }
    }

    if( ret < 0 ){
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

int chan_read_holdingregisters(int idx, int slaveaddr, int regaddr, int nb, unsigned short* regval )
{
    int rc;
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];

    if( chan->ctx == NULL){
        ret = -1;
    }else{
        modbus_set_slave(chan->ctx, slaveaddr);
        rc = modbus_read_registers( chan->ctx, regaddr, nb, regval);
        modbus_flush(chan->ctx);
        if( rc != nb ){
            chan->errcnt++;
            ret = -2;
        }

    }

    if( ret < 0 ){
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

/*
int chan_enable_dbg( int idx )
{
    int ret = 0;

    struct chan_t* chan = &mdl.chan[idx];
    if( chan->ctx != NULL ){
        chan->dbg = 1;
        modbus_set_debug( chan->ctx, 1);
    }else{
        chan->dbg = 1;
    }
    return ret;
}

int chan_disable_dbg( int idx )
{
    int ret = 0;

    struct chan_t* chan = &mdl.chan[idx];
    if( chan->ctx != NULL ){
        chan->dbg = 0;
        modbus_set_debug( chan->ctx, 0);
    }else{
        chan->dbg = 0;
    }
    return ret;
}
*/

int chan_set_dbg( int idx, int enable )
{
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];
    chan->dbg = enable;
    if( chan->ctx != NULL){
        modbus_set_debug(chan->ctx, enable);
    }
    return ret;
}

int chan_set_en( int idx, int enable )
{
    int ret = 0;
    struct chan_t* chan = &mdl.chan[idx];
    chan->en = enable;

    log_dbg("%s, idx:%d, enable:%d", __func__, idx, enable);
    return ret;
}

int chan_get_en( int idx )
{

    struct chan_t* chan = &mdl.chan[idx];
    return chan->en;
}

/* ----------------------- Defines ------------------------------------------*/


static enum ThreadState
{
    STOPPED,
    RUNNING,
    SHUTDOWN
} ePollThreadState;

static pthread_mutex_t xLock = PTHREAD_MUTEX_INITIALIZER;
//static BOOL     bDoExit;

int chan_rxrb_init(int idx)
{
    chan_ring_buffer_init(&_chan_rx_rb[idx]);
}

int chan_txrb_init(int idx)
{
    chan_ring_buffer_init(&_chan_tx_rb[idx]);
}

fmodbus_t* MB[16];
int chan_start( int idx )
{
    pthread_t xthrd;
    struct hostent *he;
    struct sockaddr_in my_addr;
    struct timeval t;
    int sock;
    struct sockaddr_can addr;
    int ret = 0;
    int rc = 0;
    struct chan_t* chan = NULL;
    char buf[128];
    struct timeval timeout = {1,0};
    int recv_own_msgs = 0;//set loop back:  1 enable 0 disable
    const UCHAR     ucSlaveID[] = { 0xAA, 0xBB, 0xCC };
    struct sockaddr_in	servaddr;
    int slaveadr = 0;

    log_dbg("%s, ++, idx:%d",__func__, idx);

    chan = &mdl.chan[idx];
    chan->ctx = NULL;
    chan->errcnt = 0;
    chan->errcnt_max = 0;
    pthread_mutex_init(&chan->mutex, NULL);
    pthread_mutex_init(&chan->mutexdat, NULL);

    if(chan->mode == CHANMODE_SERIAL){
        log_dbg("%s, idx:%d, mode:%s, dev:%s, parity:%s, baud:%d",__func__, idx, chan->szmode, chan->szdev, chan->szparity, chan->baud);
        sprintf(buf, "/dev/%s", chan->szdev);
        chan->fd = open(buf, O_RDWR);
        if(chan->fd < 0) {
            log_dbg("%s, open %s fail", __func__, buf);
            ret = -1;
        }else{
	        if(chan_set_baud(chan->fd,chan->baud) < 0){
                log_dbg("%s, set baud %s %d fail", __func__, buf,chan->baud);
                ret = -1;
            }else if( chan_set_parity(chan->fd, 8, 1, chan->szparity[0]) < 0 ){
                log_dbg("%s, set parity %s %s fail", __func__, buf, chan->szparity);
                ret = -1;
            }else{
                chan_ring_buffer_init(&_chan_tx_rb[idx]);
                chan_ring_buffer_init(&_chan_rx_rb[idx]);
                chanthrd_param[idx].idx = idx;
                if(pthread_create(&xthrd, NULL, chan_serial_rxthrd, &chanthrd_param[idx]) != 0){
                    log_dbg("%s, chan_serial_rxthrd create fail", __func__);
                    ret = -1;
                }else if( pthread_create(&xthrd, NULL, chan_serial_txthrd, &chanthrd_param[idx]) != 0 ){
                    log_dbg("%s, chan_serial_rxthrd create fail", __func__);
                    ret = -1;
                }
            }
        }
    }else if(chan->mode == CHANMODE_SOCKET_CAN){
        log_dbg("%s, idx:%d, mode:%s, dev:%s",__func__, idx, chan->szmode, chan->szdev);
        sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if(sock < 0) {
            printf("error\n");
            ret = -1;
        }else{
	        addr.can_family = AF_CAN;
	        strcpy(ifr[idx].ifr_name, chan->szdev );
	        rc = ioctl(sock, SIOCGIFINDEX, &ifr[idx]);  //get index
	        if(rc && ifr[idx].ifr_ifindex == 0) {
	            //printf("Can't get interface index for can0, code= %d, can0 ifr_ifindex value: %d, name: %s\n", ret, ifr.ifr_ifindex, ifr.ifr_name);
	            close(sock);
	            ret = -1;
	        }else{
                addr.can_ifindex = ifr[idx].ifr_ifindex;
                setsockopt(sock, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS,&recv_own_msgs, sizeof(recv_own_msgs));
                if (bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0) {
                    printf("bind error\n");
                    close(sock);
                    ret = -1;
                }else{
                    /* init success */
                    chan->sockfd = sock;
                    //chan->ring_buffer = &ring_buffer[idx];
                    //chan->ring_buffer_tx = &ring_buffer_tx[idx];
                    chan_ring_buffer_init(chan->ring_buffer);
                    chan_ring_buffer_init(chan->ring_buffer_tx);
                    chanthrd_param[idx].idx = idx;
                    if(pthread_create(&xthrd, NULL, _socketcan_readthrd, &chanthrd_param[idx]) != 0){
                        ret = -1;
                    }else if( pthread_create(&xthrd, NULL, _socketcan_writethrd, &chanthrd_param[idx]) != 0 ){
                        ret = -1;
                    }
                }
	        }
        }
    }else if(chan->mode == CHANMODE_TCPSERV_CAN){
        log_dbg("%s, idx:%d, mode:%s, host:%s, port:%d",__func__, idx, chan->szmode, chan->servip, chan->servport);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) {
            log_dbg("%s, tcpserv_can socket fail", __func__);
            ret = -1;
        }else{
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family      = AF_INET;
            servaddr.sin_port        = htons(chan->servport);
            inet_pton(AF_INET, chan->servip, &servaddr.sin_addr);
            rc = connect(sock, (SA *) &servaddr, sizeof(servaddr));
            if( rc < 0 ){
                log_dbg("%s, tcpserv_can connect fail", __func__);
                ret = -1;
            }else{
                chan->sockfd = sock;
                //chan->ring_buffer = &ring_buffer[idx];
                //chan->ring_buffer_tx = &ring_buffer_tx[idx];
                chan_ring_buffer_init(chan->ring_buffer);
                chan_ring_buffer_init(chan->ring_buffer_tx);
                chanthrd_param[idx].idx = idx;
                if(pthread_create(&xthrd, NULL, _tcpservcan_readthrd, &chanthrd_param[idx]) != 0){
                    log_dbg("%s, _tcpservcan_readthrd create fail", __func__);
                    ret = -1;
                }else if( pthread_create(&xthrd, NULL, _tcpservcan_writethrd, &chanthrd_param[idx]) != 0 ){
                    log_dbg("%s, _tcpservcan_readthrd create fail", __func__);
                    ret = -1;
                }
            }
        }
    }else if( chan->mode == CHANMODE_MODBUS_RTU ){
        sprintf(buf,"/dev/%s",chan->szdev);
        log_dbg("%s, idx:%d, mode:%s, dev:%s", __func__, idx, chan->szmode, buf);
        chan->ctx = modbus_new_rtu(buf,chan->baud,'N',8,1);
        if( chan->ctx == NULL ){
            ret = -1;
        }else if(modbus_connect(chan->ctx) == -1){
            ret = -2;
            modbus_free(chan->ctx);
        }else{
            t.tv_sec=0;
            t.tv_usec=1000000;//1000ms
            modbus_set_response_timeout(chan->ctx,&t);
         }
    }else if( chan->mode == CHANMODE_MODBUS_RTU_OVER_UDP ){
        //sprintf(buf,"/dev/%s",chn->szPort);
        chan->ctx = modbus_new_rtuudp(chan->servip, chan->servport);
        if( chan->ctx == NULL ){
            ret = -3;
        }else if(modbus_connect(chan->ctx) == -1){
            ret = -4;
            modbus_free(chan->ctx);
        }else{
            t.tv_sec=0;
            t.tv_usec=1000000;//1000ms
            modbus_set_response_timeout(chan->ctx,&t);
         }
    }else if( chan->mode == CHANMODE_MODBUS_RTU_OVER_TCP ){
        //sprintf(buf,"/dev/%s",chn->szPort);
        chan->ctx = modbus_new_rtutcp(chan->servip,chan->servport);
        if( chan->ctx == NULL ){
            ret = -5;
        }else if(modbus_connect(chan->ctx) == -1){
            ret = -6;
            modbus_free(chan->ctx);
        }else{
            t.tv_sec=0;
            t.tv_usec=1000000;//1000ms
            modbus_set_response_timeout(chan->ctx,&t);
         }
    }else if( chan->mode == CHANMODE_MODBUS_TCP ){
        chan->ctx = modbus_new_tcp(chan->servip, chan->servport);
        if( chan->ctx == NULL ){
            log_dbg("%s, modbus_new_tcp fail",__func__);
            ret = -1;
        }else if(modbus_connect(chan->ctx) == -1){
            log_dbg("%s, modbus_connect fail",__func__);
            ret = -1;
            modbus_free(chan->ctx);
        }
    }else{
        log_dbg("%s, unknown chan mode :%s",__func__, chan->szmode);
        ret = -1;
    }

leave:
    log_dbg("%s, --,idx:%d, ret:%d",__func__,idx,ret);
    return ret;
}

int chan_reset( int idx )
{
    struct timeval t;
    int ret = 0;
    char buf[32];
    struct chan_t* chan = &mdl.chan[idx];

    log_dbg("%s, ++, idx:%d",__func__, idx);

    if( strcmp(chan->szmode, "modbus_tcp") ==0 ){
        chan->rstcnt += 1;
        modbus_close(chan->ctx);
        modbus_free(chan->ctx);
        chan->ctx = modbus_new_tcp(chan->servip, chan->servport);
        if( chan->ctx == NULL ){
            ret = -1;
        }else if(modbus_connect(chan->ctx) == -1){
            ret = -2;
            modbus_free(chan->ctx);
        }else{
            log_dbg("%s, idx:%d, ctx:0x%08X init success",__func__,idx,(unsigned int)(chan->ctx));
        }

    }else{
        ret = -10;
    }

    log_dbg("%s, --, idx:%d, ret:%d",__func__, idx, ret);
    return ret;
}

int chan_txrb_used(int idx)
{
    return chan_ring_buffer_num_items(&_chan_tx_rb[idx]);
}

int chan_rxrb_used(int idx)
{
    return chan_ring_buffer_num_items(&_chan_rx_rb[idx]);
}

int chan_rxrb_dequeue_arr(int idx, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t len)
{
    return chan_ring_buffer_dequeue_arr(&_chan_rx_rb[idx], data, len);
}

void chan_txrb_queue_arr(int idx, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t size)
{
    chan_ring_buffer_queue_arr(&_chan_tx_rb[idx],  data, size);
}
