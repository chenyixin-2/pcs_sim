#include "chan.h"
#include "plt.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
extern int channbr;
extern struct chan_t chan[CHAN_NBR_MAX + 1];
static int sockfd = -1;
static struct sockaddr_in saddr, caddr;