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
#define LPWAN_GW_RUNNING_MODE       LPWAN_GW_RUNNING_MODE_AUTO_TEST


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
#define BEACON_MAX_SEQ_NUM      0x7F

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

#define LPWAN_MAX_RETRANSMIT_NUM        2
#define LPWAN_MAX_CHANNEL_BACKOFF_NUM   5

/**< @} */
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_CONFIG_H_ */
