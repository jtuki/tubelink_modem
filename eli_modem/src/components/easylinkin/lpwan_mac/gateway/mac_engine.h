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
#define SIGNAL_LPWAN_RADIO_TX_TIMEOUT                   BV(2)
#define SIGNAL_LPWAN_RADIO_TX_OK                        BV(3)

#define SIGNAL_GW_MAC_ENGINE_INIT_FINISHED              BV(4)
#define SIGNAL_GW_MAC_SEND_BEACON                       BV(5)

#define SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT        BV(6)
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
    gateway_uuid_t gateway_uuid;

    struct ringbuffer *packed_ack_delay_list[GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM];
    os_int8 cur_packed_ack_delay_list_id; /**< init value:
                                               0-GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM */

    // we use the information stored in @bcn_info to generate beacon frames.
    struct parsed_beacon_info bcn_info;
};

extern os_pid_t gl_gateway_mac_engine_pid;

void gateway_mac_engine_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_H_ */
