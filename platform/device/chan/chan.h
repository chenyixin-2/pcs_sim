#ifndef CHAN_H
#define CHAN_H

/* special address description flags for the CAN_ID */
#define CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define CAN_ERR_FLAG 0x20000000U /* error frame */

/* valid bits in CAN ID for frame formats */
#define CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

enum chanmode_t{
    CHANMODE_UNKNOWN = 0,
    CHANMODE_SERIAL,
    CHANMODE_SOCKET_CAN,
    CHANMODE_TCPSERV_CAN,
    CHANMODE_MODBUS_RTU,
    CHANMODE_MODBUS_RTU_SLAVE,
    CHANMODE_MODBUS_RTU_OVER_TCP,
    CHANMODE_MODBUS_RTU_OVER_UDP,
    CHANMODE_MODBUS_TCP,
    CHANMODE_MODBUS_TCP_SLAVE,
    CHANMODE_UDP2SERIAL,
    CHANMODE_TCP2SERIAL,
    //CHANMODE_TCP2CAN,
};

/*
    * Controller Area Network Identifier structure
    *
    * bit 0-28	: CAN identifier (11/29 bit)
    * bit 29	: error frame flag (0 = data frame, 1 = error frame)
    * bit 30	: remote transmission request flag (1 = rtr frame)
    * bit 31	: frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
*/
typedef __u32 canid_t;

/*
    * Controller Area Network Error Frame Mask structure
    *
    * bit 0-28	: error class mask (see include/linux/can/error.h)
    * bit 29-31	: set to zero
*/
typedef __u32 can_err_mask_t;

/**
    * struct can_frame - basic CAN frame structure
    * @can_id:  the CAN ID of the frame and CAN_*_FLAG flags, see above.
    * @can_dlc: the data length field of the CAN frame
    * @data:    the CAN frame payload.
*/
struct can_frame {
    canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
    __u8    can_dlc; /* data length code: 0 .. 8 */
    __u8    data[8] __attribute__((aligned(8)));
};

/* particular protocols of the protocol family PF_CAN */
#define CAN_RAW		1 /* RAW sockets */
#define CAN_BCM		2 /* Broadcast Manager */
#define CAN_TP16	3 /* VAG Transport Protocol v1.6 */
#define CAN_TP20	4 /* VAG Transport Protocol v2.0 */
#define CAN_MCNET	5 /* Bosch MCNet */
#define CAN_ISOTP	6 /* ISO 15765-2 Transport Protocol */
#define CAN_NPROTO	7
#define SOL_CAN_BASE 100

/**
    * struct sockaddr_can - the sockaddr structure for CAN sockets
    * @can_family:  address family number AF_CAN.
    * @can_ifindex: CAN network interface index.
    * @can_addr:    protocol specific address information
*/
struct sockaddr_can {
    sa_family_t can_family;
    int         can_ifindex;
    union {
        /* transport protocol class address information (e.g. ISOTP) */
        struct { canid_t rx_id, tx_id; } tp;

        /* reserved for future CAN protocols address information */
    } can_addr;
};

/**
    * struct can_filter - CAN ID based filter in can_register().
    * @can_id:   relevant bits of CAN ID which are not masked out.
    * @can_mask: CAN mask (see description)
    *
    * Description:
    * A filter matches, when
    *
    *          <received_can_id> & mask == can_id & mask
    *
    * The filter can be inverted (CAN_INV_FILTER bit set in can_id) or it can
    * filter for error frames (CAN_ERR_FLAG bit set in mask).
*/
struct can_filter {
    canid_t can_id;
    canid_t can_mask;
};

/** * The size of a ring buffer.
    * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
    * can be contained in the buffer.
    * The buffer size must be a power of two.
*/
#define RING_BUFFER_SIZE 0x100
#if (RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif



/**
    * Used as a modulo operator
    * as <tt> a % b = (a & (b ? 1)) </tt>
    * where \c a is a positive index in the buffer and
    * \c b is the (power of two) size of the buffer.
*/
#define RING_BUFFER_MASK (RING_BUFFER_SIZE-1)

/** * The type which is used to hold the size
    * and the indicies of the buffer.
    * Must be able to fit \c RING_BUFFER_SIZE .
*/
//typedef uint8_t ring_buffer_size_t;
typedef int ring_buffer_size_t;

typedef struct tag_ring_buffer_element{
    char c;
    struct can_frame frame;
}ring_buffer_element_t;

/**
  * Simplifies the use of <tt>struct ring_buffer_t</tt>.
  */
typedef struct ring_buffer_t ring_buffer_t;

/**
    * Structure which holds a ring buffer.
    * The buffer contains a buffer array
    * as well as metadata for the ring buffer.
    */
struct ring_buffer_t {
    /** Buffer memory. */
    //char buffer[RING_BUFFER_SIZE];
    ring_buffer_element_t buffer[RING_BUFFER_SIZE];
    /** Index of tail. */
    ring_buffer_size_t tail_index;
    /** Index of head. */
    ring_buffer_size_t head_index;
};

#define CAN_INV_FILTER 0x20000000U /* to be set in can_filter.can_id */

#define chan_lock(idx) pthread_mutex_lock(&mdl.chan[idx].mutex)
#define chan_unlock(idx) pthread_mutex_unlock(&mdl.chan[idx].mutex)

int chan_init( );
int chan_read_bits(int idx, int slaveaddr, int regaddr, int nb, unsigned char* dest );
int chan_write_multi_registers( int idx, int slaveaddr, int regaddr, int nb, unsigned short* regval );
int chan_read_holdingregisters( int idx, int slaveaddr, int regaddr, int nb, unsigned short* regval );
int chan_write_single_register(int idx, int slaveaddr, int regaddr, int regval );
int chan_write_bit( int idx, int slaveaddr, int addr, int status );
int chan_reset( int idx );
int chan_start( int idx );
int chan_read_input_bits( int idx, int slaveaddr, int regaddr, int nb, unsigned char* dest );
int chan_read_input_registers( int idx, int slaveaddr, int regaddr, int nb, unsigned short* dest );
int chan_set_dbg( int idx, int enable );
int chan_set_en( int idx, int enable );
int chan_get_en( int idx );
void chan_set_mbs( int idx, int mbsdevm, int mbsdevidx );
int chan_txrb_used(int idx);
int chan_rxrb_used(int idx);
int chan_rxrb_dequeue_arr(int idx, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t len);
void chan_txrb_queue_arr(int idx, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t size);
int chan_rxrb_init(int idx);
int chan_txrb_init(int idx);
#endif /* CHAN_H */
