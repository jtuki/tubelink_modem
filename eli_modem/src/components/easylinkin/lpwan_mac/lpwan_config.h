/**
 * lpwan_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef LPWAN_CONFIG_H_
#define LPWAN_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "lpwan_radio_config_sets.h"

/*---------------------------------------------------------------------------*/
/**< Gateway modem running mode selection. */
#define LPWAN_GW_RUNNING_MODE_AUTO_TEST         0   // auto initialize and print_log() enabled
#define LPWAN_GW_RUNNING_MODE_CPU_CONNECTED     1   // need external CPU to initialize gateway modem
#define LPWAN_GW_RUNNING_MODE   LPWAN_GW_RUNNING_MODE_AUTO_TEST
// #define LPWAN_GW_RUNNING_MODE   LPWAN_GW_RUNNING_MODE_CPU_CONNECTED

/**< End-device modem running mode - is virtual JOIN or real JOIN? */
#define DE_MAC_ENGINE_IS_VIRTUAL_JOIN   OS_TRUE
// #define DE_MAC_ENGINE_IS_VIRTUAL_JOIN   OS_FALSE

/*---------------------------------------------------------------------------*/
/**< Radio configuration @{ */

#define LPWAN_MAX_RADIO_CHANNELS_NUM    40  // modem's channel id range [0, 40)

extern const os_uint32 lpwan_radio_channels_list[LPWAN_MAX_RADIO_CHANNELS_NUM];
#define LPWAN_DEFAULT_RADIO_FREQUENCY (423000 * 1000)   // lpwan_radio_channels_list[0]

#define RADIO_TX_POWER_LEVELS_NUM       4   // see @lpwan_radio_tx_power_list
extern const os_int8 lpwan_radio_tx_power_list[RADIO_TX_POWER_LEVELS_NUM];

/**< @} */
/*---------------------------------------------------------------------------*/
/**< Beacon configuration @{ */

/**
 * \brief Use lollipop sequence counter for beacon seq number counting.
 * \sa http://en.wikipedia.org/wiki/Lollipop_sequence_numbering
 */
#define BEACON_MAX_SEQ_NUM      ((os_int8) 0x7F)
#define BEACON_OVERFLOW_SEQ_NUM ((os_int8) (0x80))

#define LPWAN_BEACON_PERIOD_SLOTS_NUM      128

/**
 * At most 16 classes.
 */
#define BEACON_MAX_CLASSES_NUM  16

/**
 * \brief The tx power beacon use.
 * \sa lpwan_radio_tx_power_list
 */
#define LPWAN_BEACON_TX_POWER (lpwan_radio_tx_power_list[RADIO_TX_POWER_LEVELS_NUM-1])

/**< @} */
/*---------------------------------------------------------------------------*/
/**< The parameters of end-devices' uplink message. @{ */

#define LPWAN_UNPAID_MINIMUM_ROUTINE_INTERVAL       (30 * 60)   // seconds
#define LPWAN_PAID_MINIMUM_ROUTINE_INTERVAL         (15 * 60)   // seconds

#define LPWAN_PAID_MAX_MSG_PER_BEACON_PERIOD      1

#define LPWAN_MAX_PAYLAOD_LEN           20

#define LPWAN_DE_MAX_RETRANSMIT_NUM     2   // max value: 3 (2 bits)
#define LPWAN_DE_MAX_TX_FAIL_NUM        4   // max value: 7 (2 bits)

/**< If the MCU will be idle for at least 12ms, we put it into low-power mode. */
#define LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS     12

/**< @} */
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_CONFIG_H_ */
