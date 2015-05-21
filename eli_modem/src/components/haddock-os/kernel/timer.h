/**
 * timer.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"
#include "lib/linked_list.h"
#include "kernel/ipc.h"
#include "kernel/process.h"

struct time {
    os_uint32 s;
    os_uint16 ms;
    /** Does the timer is an absolute timer?
     *  (absolute timers will not shift during sync after first sync)
     *  \sa haddock_timer_sync() */
    os_boolean is_absolute;
};

struct timer {
    struct list_head hdr;
    struct time timeout_value; /**< \sa haddock_time_tick_now */
    signal_t signal;
    os_pid_t pid;
    os_boolean is_one_shot;
};

int haddock_timer_module_init(void);
void haddock_timer_update_routine(void);
void haddock_timer_sync(const struct time *sync_time);

void haddock_get_time_tick_now(struct time *t);

typedef void (*timer_out_of_sync_callback_t) (os_uint32 delta_s, os_uint16 delta_ms); 
void haddock_timer_set_out_of_sync_callback(timer_out_of_sync_callback_t f);

struct time *haddock_check_next_timeout(void);

/** create a normal timer. */
#define os_timer_create(pid, signal, delta_ms) \
    __haddock_timer_create((pid), OS_FALSE, OS_FALSE, (signal), (delta_ms))

/** create an timer which @timeout_value is absolute. */
#define os_atimer_create(pid, signal, delta_ms) \
    __haddock_timer_create((pid), OS_TRUE, OS_FALSE, (signal), (delta_ms))

#define os_timer_create_one_shot(pid, signal, delta_ms) \
    __haddock_timer_create((pid), OS_FALSE, OS_TRUE, (signal), (delta_ms))

void os_timer_reconfig(struct timer *timer, os_pid_t pid,
                       signal_t signal, os_uint32 delta_ms);
    
int os_timer_start(struct timer *timer);
void os_timer_stop(struct timer *timer);
void os_timer_destroy(struct timer *timer);

// called in ISR each 1ms.
void __haddock_increment_time_tick_now(os_uint16 delta_ms);

struct timer *__haddock_timer_create(os_pid_t pid,
                                     os_boolean is_absolute, os_boolean is_one_shot,
                                     signal_t signal, os_uint32 delta_ms);

os_boolean haddock_debug_os_timer_list_have_loop(os_boolean is_absolute_timer_list,
                                                 os_size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */
