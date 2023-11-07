#include "plt.h"
#include "chan.h"

extern int channbr;
extern struct chan_t chan[CHAN_NBR_MAX + 1];
static int sockfd = -1;
static struct sockaddr_in saddr, caddr;