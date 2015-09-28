/**
 * lpwan_radio_config_sets.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef LPWAN_RADIO_CONFIG_SETS_H_
#define LPWAN_RADIO_CONFIG_SETS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "frame_defs/gw_beacon.h"

/*---------------------------------------------------------------------------*/
/**< Radio configuration sets. */

#define LPWAN_RADIO_CONFIG_SET_SF7      0
#define LPWAN_RADIO_CONFIG_SET_SF8      1

// #define LPWAN_RADIO_CONFIGURATION   LPWAN_RADIO_CONFIG_SET_SF7
#define LPWAN_RADIO_CONFIGURATION   LPWAN_RADIO_CONFIG_SET_SF8

#if defined (LPWAN_RADIO_CONFIGURATION) && LPWAN_RADIO_CONFIGURATION == LPWAN_RADIO_CONFIG_SET_SF7
/**< 125khz, SF7.
 * \sa global lora settings @LoRaSettings */
#define LPWAN_BEACON_PERIOD     BEACON_PERIOD_4S
#define LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS     6  // (4000/128)*6 = 187.5ms (beacon / downlink+ACK)
#define LPWAN_DE_MIN_TX_RESERVED_SLOTS              4  // (4000/128)*4 = 125ms (minimum time that should be reserved for tx)
#elif defined (LPWAN_RADIO_CONFIGURATION) && LPWAN_RADIO_CONFIGURATION == LPWAN_RADIO_CONFIG_SET_SF8
/**< 125khz, SF8.
 * \sa global lora settings @LoRaSettings */
#define LPWAN_BEACON_PERIOD     BEACON_PERIOD_8S
#define LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS     6  // (8000/128)*6 = 375ms (beacon / downlink+ACK)
#define LPWAN_DE_MIN_TX_RESERVED_SLOTS              4  // (8000/128)*4 = 250ms (minimum time that should be reserved for tx)
#endif

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_RADIO_CONFIG_SETS_H_ */
