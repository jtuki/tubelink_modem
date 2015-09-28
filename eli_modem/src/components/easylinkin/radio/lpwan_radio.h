/**
 * lpwan_radio.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef LPWAN_RADIO_H_
#define LPWAN_RADIO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"
#include "kernel/process.h"
#include "kernel/timer.h"
#include "radio_config.h"

extern os_uint8 lpwan_radio_rx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];

/**< \sa lpwan_radio_tx() */
#define LPWAN_RADIO_ERR_TX_LEN_INVALID              -1
/**< \sa lpwan_radio_start_rx() */
#define LPWAN_RADIO_ERR_START_RX_DURING_RX          -1
/**< \sa lpwan_radio_read() */
#define LPWAN_RADIO_ERR_READ_BUFFER_LEN_INVALID     -1

#define LPWAN_RADIO_TX_MAX_LEN      128

void lpwan_radio_register_radio_controller_pid(os_pid_t pid);

void lpwan_radio_init(void);
void lpwan_radio_routine(void);

os_int8 lpwan_radio_tx(const os_uint8 frame[], os_uint16 len);
os_int8 lpwan_radio_start_rx(void);
os_int8 lpwan_radio_stop_rx(void);
os_int8 lpwan_radio_read(os_uint8 buffer[], os_uint16 len);

void lpwan_radio_sleep(void);
void lpwan_radio_wakeup(void);

void lpwan_radio_change_base_frequency(os_uint32 freq);

os_int16 lpwan_radio_get_rssi(void);
os_int16 lpwan_radio_get_snr(void);

struct lpwan_last_rx_frame_rssi_snr {
    os_int16 rssi;
    os_int16 snr;
};

struct lpwan_last_rx_frame_rssi_snr lpwan_radio_get_last_rx_rssi_snr(void);

struct lpwan_last_rx_frame_time {
    struct time tx; /**< the _approximated_ time when the last received frame is sent */
    struct time rx; /**< the time when the last received frame is received */
};

void lpwan_radio_get_last_rx_frame_time(struct lpwan_last_rx_frame_time *t);

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_RADIO_H_ */
