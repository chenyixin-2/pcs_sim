#include "log.h"
#include "misc.h"
#include "plt.h"
#include "sta.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

static int appl_init()
{
    int ret = 0;

    if (0)
    { // sta_init() != 0
        ret = -1;
    }
    else if (cloud_init() != 0)
    { /* cloud_init() != 0 */
        ret = -1;
    }

    log_dbg("%s, ret:%d", __func__, ret);
    return ret;
}

static int appl_run()
{
    while (1)
    {
        usleep(90000);
        cloud_exe();
    }
}

extern int DAEMON;
int appl_main(void)
{
    int ret = 0;

    syslog(LOG_INFO, "%s, ++", __func__);

    if (DAEMON == 1)
    {
        if (misc_daemon(true, false) != 0)
        {
            log_dbg("%s, become daemon fail", __func__);
            ret = -1;
            goto leave;
        }
    }

    if (plt_init_stp1() != 0)
    {
        log_dbg("%s, plt_init_stp1 fail", __func__);
        ret = -1;
    }
    else if (appl_init() != 0)
    {
        log_dbg("%s, appl_init fail", __func__);
        ret = -1;
    }
    else if (plt_init_stp2() != 0)
    {
        log_dbg("%s, appl_init_stp2 fail", __func__);
        ret = -1;
    }
    else
    {
        appl_run();
    }

leave:
    syslog(LOG_INFO, "%s, --, ret:%d", __func__, ret);
    return ret;
}