/**
 * process_test_end_device_app.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"
#include "lib/assert.h"
#include "lpwan_mac/end_device/mac_engine.h"

#include "apps/simple_log.h"

#include "process_test_end_device_app.h"


static signal_bv_t proc_test_end_device_app_entry(os_pid_t pid, signal_bv_t signal);

os_pid_t gl_proc_pid_test_user_app;

haddock_process("proc_test_end_device_app");

static struct timer *period_uplink_msg_timer = NULL;

#define UPLINK_PERIOD_DELAY_MS (1000 * hdk_randr(20, 40))

void proc_test_end_device_app_init(os_uint8 priority)
{
    struct process *proc_test_user_app = \
            process_create(proc_test_end_device_app_entry, priority);
    gl_proc_pid_test_user_app = proc_test_user_app->_pid;

    period_uplink_msg_timer = os_timer_create(this->_pid,
                                    SIGNAL_USER_APP_UPLINK_MSG_PERIODICALLY,
                                    UPLINK_PERIOD_DELAY_MS);
    haddock_assert(period_uplink_msg_timer);
    os_timer_start(period_uplink_msg_timer);
    process_sleep();
}

static signal_bv_t proc_test_end_device_app_entry(os_pid_t pid, signal_bv_t signal)
{
    static os_uint8 uplink_msg_seq_id = 0;

    // first byte for @uplink_msg_seq_id
    static os_uint8 uplink_msg_buffer[20] = "-hi, easylinkin!";

    if (signal & SIGNAL_USER_APP_UPLINK_MSG_PERIODICALLY) {
        enum device_mac_states _state = mac_info_get_mac_states();
        // we only try to send message when the end-device has joined the network.
        if (_state == DE_MAC_STATES_JOINED) {
            uplink_msg_buffer[0] = uplink_msg_seq_id;
            if (0 == device_mac_send_msg(DEVICE_MSG_TYPE_EVENT,
                                         uplink_msg_buffer,
                                         sizeof("-hi, easylinkin!")-1)) {
                uplink_msg_seq_id += 1;
            }
        }

        os_timer_reconfig(period_uplink_msg_timer,
                          this->_pid,
                          SIGNAL_USER_APP_UPLINK_MSG_PERIODICALLY,
                          UPLINK_PERIOD_DELAY_MS);
        os_timer_start(period_uplink_msg_timer);
        
        process_sleep();
        return signal ^ SIGNAL_USER_APP_UPLINK_MSG_PERIODICALLY;
    }
    // unknown signal? discard.
    return 0;
}
