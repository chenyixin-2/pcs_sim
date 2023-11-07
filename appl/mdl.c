#include "plt.h"
#include "mdl.h"

void mdl_sim()
{
}

static void *mdl_thrd_main(void *param)
{

    while (1)
    {
        mdl_sim();
        usleep(10000); /* 10ms */
    }
    return NULL;
}

int mdl_init()
{
    log_dbg("%s, ++", __func__);
    int ret = 0;
    pthread_t xthrd;
    if (pthread_create(&xthrd, NULL, mdl_thrd_main, NULL) != 0)
    {
        log_dbg("%s, pthread_create fail", __func__);
        ret = -1;
    }
    log_dbg("%s, --, ret: %d", __func__, ret);
    return ret;
}