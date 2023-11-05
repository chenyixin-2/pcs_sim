#include "plt.h"

static struct state_t tbmqtt_states[] = {
    { SMST_OFFLINE, "offline" },
    { SMST_READY, "ready" },
};

static struct err_t tbmqtt_errs[] = {
    { TBMQTTERR_NONE, "none" },

    // offline
    { TBMQTTERR_OFFLINE_PWRUP, "pwrup" },
};

int tbmqtt_sm_init()
{
    struct statemachine_t* sm = &tbmqtt.sm;

    sm_reset_timing(sm, 20, 20);
    sm->states = tbmqtt_states;
    sm->state_nbr = sizeof(tbmqtt_states)/sizeof(struct state_t);
    sm->errs = tbmqtt_errs;
    sm->err_nbr = sizeof(tbmqtt_errs)/sizeof(struct err_t);
    sm_set_state( sm, SMST_OFFLINE, TBMQTTERR_OFFLINE_PWRUP );

    return 0;
}





