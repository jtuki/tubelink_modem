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
#include "lpwan_radio_config_sets.h"

/*---------------------------------------------------------------------------*/
/**< configurations @{ */

#if defined (LPWAN_RADIO_CONFIGURATION) && LPWAN_RADIO_CONFIGURATION == LPWAN_RADIO_CONFIG_SET_SF7
#define RADIO_CONTROLLER_MIN_TRY_TX_DURATION        150     // at least 150ms should be reserved \sa LPWAN_DE_MIN_TX_RESERVED_SLOTS
#define RADIO_CONTROLLER_TRY_TX_LISTEN_IN_ADVANCE   30      // listen before tx.
                                                            // should be less than @RADIO_CONTROLLER_MIN_TRY_TX_DURATION.
#define RADIO_CONTROLLER_RX_DURATION_MIN_SPAN       20
#elif defined (LPWAN_RADIO_CONFIGURATION) && LPWAN_RADIO_CONFIGURATION == LPWAN_RADIO_CONFIG_SET_SF8
#define RADIO_CONTROLLER_MIN_TRY_TX_DURATION        300     // at least 300ms should be reserved \sa LPWAN_DE_MIN_TX_RESERVED_SLOTS
#define RADIO_CONTROLLER_TRY_TX_LISTEN_IN_ADVANCE   60      // listen before tx.
                                                            // should be less than @RADIO_CONTROLLER_MIN_TRY_TX_DURATION
#define RADIO_CONTROLLER_RX_DURATION_MIN_SPAN       40
#endif

#define RADIO_CONTROLLER_RADIO_ROUTINE_CHECK_PERIOD     3 // radio routine checker period: 3ms
#define RADIO_CONTROLLER_MAX_TIMER_DELTA_MS             (((os_uint32)1)<<31)

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

#define switch_rlc_caller() rlc_register_caller(this->_pid)
void rlc_register_caller(os_pid_t caller_pid);

void rlc_reset(void);

/**
 * \return
 *  RADIO_CONTROLLER_TX_ERR_NOT_IDLE:
 *      if current state is not RADIO_STATES_IDLE.
 *  RADIO_CONTROLLER_TX_ERR_INVALID_LEN:
 *      if len is too long.
 *  0 if ok.
 */
os_int8 radio_controller_tx(const os_uint8 frame[], os_uint8 len,
                            os_uint16 try_tx_duration);
os_int8 radio_controller_tx_immediately(const os_uint8 frame[], os_uint8 len);

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

/**
 * RLC's rx buffer.
 * \note gl_radio_rx_buffer[0] is the received length.
 * \sa _radio_rx_buffer
 */
extern os_uint8 *gl_radio_rx_buffer;

#ifdef __cplusplus
}
#endif

#endif /* RADIO_CONTROLLER_H_ */
