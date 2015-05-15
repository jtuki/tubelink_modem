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
#include "radio_config.h"

extern os_uint8 lpwan_radio_rx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];

/**< \sa lpwan_radio_tx() */
#define LPWAN_RADIO_ERR_TX_LEN_INVALID              -1
/**< \sa lpwan_radio_start_rx() */
#define LPWAN_RADIO_ERR_START_RX_DURING_RX          -1
/**< \sa lpwan_radio_read() */
#define LPWAN_RADIO_ERR_READ_BUFFER_LEN_INVALID     -1

#define LPWAN_RADIO_TX_MAX_LEN      128

void lpwan_radio_init(void);
void lpwan_radio_routine(void);

os_int8 lpwan_radio_tx(const os_uint8 frame[], os_uint16 len);
os_int8 lpwan_radio_start_rx(void);
os_int8 lpwan_radio_stop_rx(void);
os_int8 lpwan_radio_read(os_uint8 buffer[], os_uint16 len);

void lpwan_radio_sleep(void);
void lpwan_radio_wakeup(void);

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_RADIO_H_ */
