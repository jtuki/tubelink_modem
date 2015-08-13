/**
 * radio_controller.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef RADIO_CONTROLLER_H_
#define RADIO_CONTROLLER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel/process.h"
#include "haddock_types.h"
#include "lib/hdk_utilities.h"
    
#include "radio/lpwan_radio.h"
#include "radio/radio_config.h"

/*---------------------------------------------------------------------------*/
/**< configurations @{ */

#define RADIO_CONTROLLER_MIN_TRY_TX_DURATION        20 // at least 20 ms should be reserved
#define RADIO_CONTROLLER_TRY_TX_LISTEN_IN_ADVANCE   10 // should be less than @RADIO_CONTROLLER_MIN_TRY_TX_DURATION

#define RADIO_CONTROLLER_RX_DURATION_MIN_SPAN       20

#define RADIO_CONTROLLER_RADIO_ROUTINE_CHECK_PERIOD 3 // radio routine checker period: 3ms

#define RADIO_CONTROLLER_MAX_TIMER_DELTA_MS         (((os_uint32)1)<<31)

/**< @} */
/*---------------------------------------------------------------------------*/
/**< signals @{ */

#define SIGNAL_LPWAN_RADIO_COMBINED_ALL                 ((os_uint32) 0x1F)
#define SIGNAL_LPWAN_RADIO_RX_TIMEOUT                   BV(0)
#define SIGNAL_LPWAN_RADIO_RX_OK                        BV(1)
#define SIGNAL_LPWAN_RADIO_RX_CRC_ERROR                 BV(2)
#define SIGNAL_LPWAN_RADIO_TX_TIMEOUT                   BV(3)
#define SIGNAL_LPWAN_RADIO_TX_OK                        BV(4)

#define SIGNAL_RADIO_CONTROLLER_INITED                  BV(6)
#define SIGNAL_RADIO_CONTROLLER_TX_RAND_WAIT_TIMEOUT    BV(7)
#define SIGNAL_RADIO_CONTROLLER_TX_START                BV(8)
#define SIGNAL_RADIO_CONTROLLER_RX_DURATION_TIMEOUT     BV(9)

#define SIGNAL_RADIO_CONTROLLER_ROUTINE_CHECK_TIMER     BV(10)

/**< @} */
/*---------------------------------------------------------------------------*/

enum radio_controller_states {
    RADIO_STATES_IDLE = 0,
    RADIO_STATES_TX,        /**< is tx-ing */
    RADIO_STATES_TX_CCA,    /**< get ready to tx, perform CCA */
    RADIO_STATES_TX_ING,    /**< \sa lpwan_radio_tx */
    RADIO_STATES_RX,        /**< is rx-ing */
};

enum radio_controller_states get_radio_controller_states(void);

#define RADIO_CONTROLLER_TX_ERR_NOT_IDLE       -1
#define RADIO_CONTROLLER_TX_ERR_INVALID_LEN    -2

void radio_controller_register_mac_engine(os_pid_t mac_engine_pid);

/**
 * \return
 *  RADIO_CONTROLLER_TX_ERR_NOT_IDLE:
 *      if current state is not RADIO_STATES_IDLE.
 *  RADIO_CONTROLLER_TX_ERR_INVALID_LEN:
 *      if len is too long.
 *  0 if ok.
 */
os_int8 radio_controller_tx(os_uint8 frame[], os_uint8 len,
                            os_uint16 try_tx_duration);

#define RADIO_CONTROLLER_RX_ERR_NOT_IDLE    -1

/**
 * \return
 *  RADIO_CONTROLLER_RX_ERR_NOT_IDLE:
 *      if current state is not RADIO_STATES_IDLE.
 *  0 if ok.
 *
 * \remark
 *  As to @radio_controller_rx_continuously(), the radio will continue until
 *  be explicitly called to stop (\sa radio_controller_rx_stop()).
 *  As to @radio_controller_rx(), the radio will automatically stop when it
 *  has received a frame.
 */
os_int8 radio_controller_rx_continuously(os_uint16 rx_duration);
os_int8 radio_controller_rx(os_uint16 rx_duration);

/**
 * \return
 *  -1: if current state is not RADIO_STATES_RX.
 *   0: else, return 0.
 */
os_int8 radio_controller_rx_stop(void);

void radio_controller_init(os_uint8 priority);

extern os_uint8 *radio_rx_buffer; /** \ref __radio_rx_buffer */

#ifdef __cplusplus
}
#endif

#endif /* RADIO_CONTROLLER_H_ */
