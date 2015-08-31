/**
 * mac_engine.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef MAC_ENGINE_H_
#define MAC_ENGINE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "lpwan_utils.h"
#include "protocol_utils/parse_beacon.h"
  
#include "kernel/process.h"

#include "lib/ringbuffer.h"

/**< signals @{ */
    
/** radio related signals */
#define SIGNAL_LPWAN_RADIO_RX_TIMEOUT                   BV(0)
#define SIGNAL_LPWAN_RADIO_RX_OK                        BV(1)
#define SIGNAL_LPWAN_RADIO_RX_CRC_ERROR                 BV(2)
#define SIGNAL_LPWAN_RADIO_TX_TIMEOUT                   BV(3)
#define SIGNAL_LPWAN_RADIO_TX_OK                        BV(4)

#define SIGNAL_GW_MAC_SEND_BEACON                       BV(5)
#define SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT        BV(6)

/** \ref Driven by MAC engine driver's signals.
 * \sa @SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE etc */
#define SIGNAL_GW_MAC_ENGINE_START                      BV(10)
#define SIGNAL_GW_MAC_ENGINE_STOP                       BV(11)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_INFO            BV(12)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_MODEM_CHANNEL       BV(13)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_CLASSES_NUM     BV(14)


/** radio controller related signals */
#define SIGNAL_RLC_TX_OK                                BV(20)
#define SIGNAL_RLC_TX_TIMEOUT                           BV(21)
#define SIGNAL_RLC_TX_CCA_FAILED                        BV(22)
#define SIGNAL_RLC_TX_CCA_CRC_FAIL                      BV(23)
#define SIGNAL_RLC_RX_DURATION_TIMEOUT                  BV(24)
#define SIGNAL_RLC_RX_OK                                BV(25)

/**< @} */

/**< configurations @{ */
#define GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM            2   // the maxium packed ack delay
#define GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD        10  // the max uplink messages
                                                            // within a beacon period

#define GATEWAY_MAC_RADIO_PERIODICAL_CHECK_INTERVAL     2   // 2ms
/**< @} */

#define GW_MAC_ENGINE_MAX_TIMER_DELTA_MS        (((os_uint32)1)<<31)

struct lpwan_gateway_mac_info {
    short_addr_t gateway_cluster_addr;
    modem_uuid_t gateway_modem_uuid;

    os_uint8 modem_channel; /**< \sa LPWAN_MAX_RADIO_CHANNELS_NUM */

    struct ringbuffer *packed_ack_delay_list[GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM];
    os_int8 cur_packed_ack_delay_list_id; /**< init value:
                                               0-GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM */

    // we use the information stored in @bcn_info to generate beacon frames.
    struct parsed_beacon_info bcn_info;
};

extern os_pid_t gl_gateway_mac_engine_pid;

void gateway_mac_engine_init(os_uint8 priority);

os_boolean is_gw_mac_engine_started(void);
const modem_uuid_t *gw_mac_info_get_modem_uuid(void);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_H_ */
