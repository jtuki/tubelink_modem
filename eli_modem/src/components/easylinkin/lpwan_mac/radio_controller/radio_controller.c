/**
 * radio_controller.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lib/assert.h"

#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"
#include "kernel/power_manager.h"

#if defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
#include "lpwan_mac/end_device/mac_engine.h"
#elif defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
#include "lpwan_mac/gateway/mac_engine.h"
#endif

#include "lpwan_config.h"
#include "radio_controller.h"

static enum radio_controller_states gl_radio_controller_state;
static signal_bv_t radio_controller_entry(os_pid_t pid, signal_bv_t signal);

static void start_radio_check_timer(void);
static void stop_radio_check_timer(void);

static void rlc_wakeup_init_callback(void);
static os_int8 rlc_sleep_cleanup_callback(void);

#define put_radio_to_sleep() do {         \
    lpwan_radio_stop_rx();                \
    stop_radio_check_timer();             \
    process_sleep();                      \
} while (0)

os_pid_t gl_radio_controller_pid;
haddock_process("radio_controller_proc");

static os_pid_t gl_registered_caller_pid = PROCESS_ID_RESERVED;

static struct timer *radio_controller_timeout_timer; // used both for tx_rand_delay and rx_continuously
static struct timer *radio_check_timer;

/**
 * Only a single frame can be tx at a time. (frame == NULL if no frame is tx-ing)
 */
static struct {
    os_uint8 len;
    const os_uint8 *frame;
    struct time try_tx_time;
    os_uint16 try_tx_duration;
} gl_cur_tx_frame;

static struct {
    os_boolean is_continuous;
} gl_radio_rx_config;

static struct {
    os_uint8 _padding1, _padding2, _padding3;
    os_uint8 buffer_size; // for back-level compatibility
    os_uint8 buffer[LPWAN_RADIO_RX_BUFFER_MAX_LEN] __attribute__((aligned (4)));
} _radio_rx_buffer;

os_uint8 *gl_radio_rx_buffer;

void radio_controller_init(os_uint8 priority)
{
    struct process *proc_radio_controller = process_create(radio_controller_entry,
                                                           priority);
    haddock_assert(proc_radio_controller);
    gl_radio_controller_pid = proc_radio_controller->_pid;

    gl_radio_rx_buffer = (os_uint8 *) &_radio_rx_buffer.buffer_size;

    radio_controller_timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                                     RADIO_CONTROLLER_MAX_TIMER_DELTA_MS);
    radio_check_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                        RADIO_CONTROLLER_MAX_TIMER_DELTA_MS);

#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
    proc_radio_controller->wakeup_init_callback   = rlc_wakeup_init_callback;
    proc_radio_controller->sleep_cleanup_callback = rlc_sleep_cleanup_callback;
#else
    (void) rlc_wakeup_init_callback;
    (void) rlc_sleep_cleanup_callback;
#endif

    /** initialize the radio interface */
    lpwan_radio_register_radio_controller_pid(gl_radio_controller_pid);
    lpwan_radio_init();

    os_ipc_set_signal(this->_pid, SIGNAL_RADIO_CONTROLLER_INITED);
}

/**
 * The corresponding signal will send to the @caller_pid.
 */
void rlc_register_caller(os_pid_t caller_pid)
{
    gl_registered_caller_pid = caller_pid;
}

static signal_bv_t radio_controller_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    if (signal & SIGNAL_RADIO_CONTROLLER_ROUTINE_CHECK_TIMER) {
        start_radio_check_timer();
        lpwan_radio_routine();

        return signal ^ SIGNAL_RADIO_CONTROLLER_ROUTINE_CHECK_TIMER;
    }

    /**< Handle the radio interface signals @{ */

    if ((signal & SIGNAL_LPWAN_RADIO_COMBINED_ALL) == 0) {
        goto radio_controller_signals_handling;
    }

    switch ((int) gl_radio_controller_state) {
    case RADIO_STATES_RX:
        if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {
            lpwan_radio_read(gl_radio_rx_buffer, 1+LPWAN_RADIO_RX_BUFFER_MAX_LEN);

            haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
            os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_RX_OK);

            if (gl_radio_rx_config.is_continuous) {
                lpwan_radio_start_rx();
            } else {
                radio_controller_rx_stop();
            }
            return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
        }
        if (signal & SIGNAL_LPWAN_RADIO_RX_CRC_ERROR) {
            lpwan_radio_start_rx();
            return signal ^ SIGNAL_LPWAN_RADIO_RX_CRC_ERROR;
        }
        if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
            lpwan_radio_start_rx();
            return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
        }
        __should_never_fall_here();
        break;
    case RADIO_STATES_TX_CCA:
        if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {
            // we cannot send the frame now, notify the mac engine.
            lpwan_radio_read(gl_radio_rx_buffer, 1+LPWAN_RADIO_RX_BUFFER_MAX_LEN);

            haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
            os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_TX_CCA_FAILED);

            gl_cur_tx_frame.frame = NULL;
            gl_radio_controller_state = RADIO_STATES_IDLE;
            put_radio_to_sleep();

            return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
        }
        if (signal & SIGNAL_LPWAN_RADIO_RX_CRC_ERROR) {
            // we regard it as CCA failed too.
            haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
            os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_TX_CCA_CRC_FAIL);

            gl_cur_tx_frame.frame = NULL;
            gl_radio_controller_state = RADIO_STATES_IDLE;
            put_radio_to_sleep();

            return signal ^ SIGNAL_LPWAN_RADIO_RX_CRC_ERROR;
        }
        if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
            // we can send the frame now!
            lpwan_radio_tx(gl_cur_tx_frame.frame, gl_cur_tx_frame.len);
            gl_radio_controller_state = RADIO_STATES_TX_ING;
            return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
        }
        __should_never_fall_here();
        break;
    case RADIO_STATES_TX_ING:
        if (signal & SIGNAL_LPWAN_RADIO_TX_OK) {
            haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
            os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_TX_OK);

            gl_cur_tx_frame.frame = NULL;
            gl_radio_controller_state = RADIO_STATES_IDLE;
            put_radio_to_sleep();

            return signal ^ SIGNAL_LPWAN_RADIO_TX_OK;
        }
        if (signal & SIGNAL_LPWAN_RADIO_TX_TIMEOUT) {
            haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
            os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_TX_TIMEOUT);

            gl_cur_tx_frame.frame = NULL;
            gl_radio_controller_state = RADIO_STATES_IDLE;
            put_radio_to_sleep();

            return signal ^ SIGNAL_LPWAN_RADIO_TX_TIMEOUT;
        }
        __should_never_fall_here();
        break;
    case RADIO_STATES_IDLE:
        print_log(LOG_INFO, "rlc(IDLE) rx signal (race condition)");
        break;
    case RADIO_STATES_TX:
    default:
        __should_never_fall_here();
        break;
    }

    /**< @} */

radio_controller_signals_handling:

    if (signal & SIGNAL_RADIO_CONTROLLER_INITED) {

        gl_radio_controller_state = RADIO_STATES_IDLE;
        put_radio_to_sleep();

        return signal ^ SIGNAL_RADIO_CONTROLLER_INITED;
    }

    if (signal & SIGNAL_RADIO_CONTROLLER_TX_START) {
        haddock_assert(gl_radio_controller_state == RADIO_STATES_TX);

        os_uint32 _rand_tx_delay = \
                hdk_randr(10, (os_uint32) gl_cur_tx_frame.try_tx_duration - RADIO_CONTROLLER_TRY_TX_LISTEN_IN_ADVANCE);
        os_timer_reconfig(radio_controller_timeout_timer, this->_pid,
                          SIGNAL_RADIO_CONTROLLER_TX_RAND_WAIT_TIMEOUT, _rand_tx_delay);
        os_timer_start(radio_controller_timeout_timer);

        if (_rand_tx_delay > LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS)
            put_radio_to_sleep();

        print_log(LOG_INFO, "rlc_tx: rand_delay (%ldms)", _rand_tx_delay);
        return signal ^ SIGNAL_RADIO_CONTROLLER_TX_START;
    }

    if (signal & SIGNAL_RADIO_CONTROLLER_TX_RAND_WAIT_TIMEOUT) {
        haddock_assert(gl_radio_controller_state == RADIO_STATES_TX);

        gl_radio_controller_state = RADIO_STATES_TX_CCA;
        lpwan_radio_start_rx();
        start_radio_check_timer();

        return signal ^ SIGNAL_RADIO_CONTROLLER_TX_RAND_WAIT_TIMEOUT;
    }

    if (signal & SIGNAL_RADIO_CONTROLLER_RX_DURATION_TIMEOUT) {
        lpwan_radio_stop_rx();
        haddock_assert(gl_registered_caller_pid != PROCESS_ID_RESERVED);
        os_ipc_set_signal(gl_registered_caller_pid, SIGNAL_RLC_RX_DURATION_TIMEOUT);

        gl_radio_controller_state = RADIO_STATES_IDLE;
        put_radio_to_sleep();

        return signal ^ SIGNAL_RADIO_CONTROLLER_RX_DURATION_TIMEOUT;
    }

    // discard unknown signal.
    return 0;
}

enum radio_controller_states get_radio_controller_states(void)
{
    return gl_radio_controller_state;
}

/**
 * Try to tx frame immediately.
 * \sa radio_controller_tx()
 *
 * If the radio controller's state is CCA and receive the RX_TIMEOUT signal, tx
 * will be immediately launched.
 * \sa RADIO_STATES_TX_CCA
 * \sa SIGNAL_LPWAN_RADIO_RX_TIMEOUT
 */
os_int8 radio_controller_tx_immediately(const os_uint8 frame[], os_uint8 len)
{
    haddock_assert(gl_radio_controller_state == RADIO_STATES_IDLE
                   && len <= LPWAN_RADIO_TX_BUFFER_MAX_LEN);
    haddock_assert(gl_cur_tx_frame.frame == NULL);

    gl_cur_tx_frame.frame = frame;
    gl_cur_tx_frame.len = len;

    // we use the short-cut to immediately launch the TX.
    gl_radio_controller_state = RADIO_STATES_TX_CCA;
    os_ipc_set_signal(this->_pid, SIGNAL_LPWAN_RADIO_RX_TIMEOUT);

    return 0;
}

/**
 * Try to send @frame during @try_tx_duration:
 * random delay first, then perform CCA, then tx the frame.
 */
os_int8 radio_controller_tx(const os_uint8 frame[], os_uint8 len,
                            os_uint16 try_tx_duration)
{
    haddock_assert(gl_radio_controller_state == RADIO_STATES_IDLE
                   && len <= LPWAN_RADIO_TX_BUFFER_MAX_LEN);

    haddock_assert(gl_cur_tx_frame.frame == NULL);
    haddock_assert(try_tx_duration == 0
                   || try_tx_duration >= RADIO_CONTROLLER_MIN_TRY_TX_DURATION);


    if (try_tx_duration == 0) {
        radio_controller_tx_immediately(frame, len);
    } else {
        gl_cur_tx_frame.frame = frame;
        gl_cur_tx_frame.len = len;
        gl_cur_tx_frame.try_tx_duration = try_tx_duration;
        haddock_get_time_tick_now(& gl_cur_tx_frame.try_tx_time);

        gl_radio_controller_state = RADIO_STATES_TX;
        os_ipc_set_signal(this->_pid, SIGNAL_RADIO_CONTROLLER_TX_START);
    }

    return 0;
}

/**
 * \sa radio_controller_rx_continuously()
 * \sa radio_controller_rx()
 */
static os_int8 _radio_controller_rx_frame(os_uint16 rx_duration,
                                          os_boolean is_continuous)
{
    if (gl_radio_controller_state != RADIO_STATES_IDLE)
        return RADIO_CONTROLLER_RX_ERR_NOT_IDLE;

    haddock_assert(rx_duration >= RADIO_CONTROLLER_RX_DURATION_MIN_SPAN);

    os_timer_reconfig(radio_controller_timeout_timer, this->_pid,
                      SIGNAL_RADIO_CONTROLLER_RX_DURATION_TIMEOUT,
                      rx_duration);
    os_timer_start(radio_controller_timeout_timer);

    lpwan_radio_start_rx();
    start_radio_check_timer();

    gl_radio_controller_state = RADIO_STATES_RX;
    gl_radio_rx_config.is_continuous = is_continuous;

    return 0;
}

os_int8 radio_controller_rx_continuously(os_uint16 rx_duration)
{
    return _radio_controller_rx_frame(rx_duration, OS_TRUE);
}

os_int8 radio_controller_rx(os_uint16 rx_duration)
{
    return _radio_controller_rx_frame(rx_duration, OS_FALSE);
}

os_int8 radio_controller_rx_stop(void)
{
    if (gl_radio_controller_state != RADIO_STATES_RX)
        return -1;

    os_timer_stop(radio_controller_timeout_timer);

    gl_radio_controller_state = RADIO_STATES_IDLE;
    put_radio_to_sleep();

    return 0;
}

static void start_radio_check_timer(void)
{
    os_timer_reconfig(radio_check_timer, this->_pid,
                      SIGNAL_RADIO_CONTROLLER_ROUTINE_CHECK_TIMER,
                      RADIO_CONTROLLER_RADIO_ROUTINE_CHECK_PERIOD);
    os_timer_start(radio_check_timer);
}

static void stop_radio_check_timer(void)
{
    os_timer_stop(radio_check_timer);
}

static void rlc_wakeup_init_callback(void)
{
    lpwan_radio_wakeup();
}

static os_int8 rlc_sleep_cleanup_callback(void)
{
    lpwan_radio_sleep();
    return 0;
}

/**
 * Reset radio link controller layer, stop all possible timers and return to
 * IDLE state.
 * \sa btracker_reset()
 */
void rlc_reset(void)
{
    gl_radio_controller_state = RADIO_STATES_IDLE;
    put_radio_to_sleep();

    gl_cur_tx_frame.frame = NULL;
    os_timer_stop(radio_controller_timeout_timer);
}
