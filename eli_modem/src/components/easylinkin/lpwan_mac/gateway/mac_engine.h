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

enum gw_mac_states {
    GW_MAC_STATE_INITED = 0,            /**< inited, MAC engine has not yet started */
    GW_MAC_STATE_TX_BEACON,             /**< beacon period: tx beacon */
    GW_MAC_STATE_TX_DOWNLINK,           /**< beacon period: tx pending downlink frames */
    // GW_MAC_STATE_RX_DOWNLINK_ACK,    /**< not used currently */
    GW_MAC_STATE_RX_UPLINK,             /**< beacon period: rx uplink frames */
};

/**< signals @{ */
    
/** \ref Driven by MAC engine driver's signals.
 * \sa @SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE etc */
#define SIGNAL_GW_MAC_DRIVER_SIGNALS                    ((os_uint32) 0x1F)
#define SIGNAL_GW_MAC_ENGINE_START                      BV(0)
#define SIGNAL_GW_MAC_ENGINE_STOP                       BV(1)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_INFO            BV(2)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_MODEM_CHANNEL       BV(3)
#define SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_CLASSES_NUM     BV(4)

#define SIGNAL_GW_MAC_SEND_BEACON                       BV(10)
#define SIGNAL_GW_MAC_TX_DOWNLINK_FRAME                 BV(11)
#define SIGNAL_GW_MAC_BEGIN_RX_UPLINK                   BV(12)

/** radio controller related signals */
#include "radio_controller/rlc_callback_signals.h"      // BV(25)~BV(30)

/**< @} */

/**< configurations @{ */
#define GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM            2   // the maxium packed ack delay
#define GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD        10  // the max uplink messages
                                                            // within a beacon period
#define GATEWAY_DOWNLINK_FRAME_BUF_NUM  \
        (GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM * GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD)

#define GATEWAY_MAC_RADIO_PERIODICAL_CHECK_INTERVAL     2   // 2ms
/**< @} */

#define GW_MAC_ENGINE_MAX_TIMER_DELTA_MS        (((os_uint32)1)<<31)

struct lpwan_gateway_mac_info {
    short_addr_t gateway_cluster_addr;
    modem_uuid_t gateway_modem_uuid;

    os_uint8 modem_channel; /**< \sa LPWAN_MAX_RADIO_CHANNELS_NUM */

    // we use the information stored in @bcn_info to generate beacon frames.
    struct parsed_beacon_info bcn_info;
};

extern os_pid_t gl_gateway_mac_engine_pid;

void gateway_mac_engine_init(os_uint8 priority);

os_boolean is_gw_mac_engine_started(void);

os_uint8 gw_mac_info_get_packed_ack_delay_num(void);
const modem_uuid_t *gw_mac_info_get_modem_uuid(void);

os_int8 gw_mac_prepare_tx_join_confirm(const modem_uuid_t *dest_uuid,
                               const struct gw_join_confirmed *join_response,
                               os_int8 tx_bcn_seq_id);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_H_ */
