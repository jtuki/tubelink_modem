/**
 * mac_engine_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef MAC_ENGINE_CONFIG_H_
#define MAC_ENGINE_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LPWAN_DEVICE_MAC_PENDING_TX_FRAME_MAX_NUM   2
#define LPWAN_DEVICE_MAC_WAIT_ACK_FRAME_MAX_NUM     3

#define LPWAN_DEVICE_MAC_UPLINK_BUFFER_MAX_NUM          \
            (LPWAN_DEVICE_MAC_PENDING_TX_FRAME_MAX_NUM  \
             + LPWAN_DEVICE_MAC_WAIT_ACK_FRAME_MAX_NUM)

#define LPWAN_DEVICE_MAC_UPLINK_MTU             40

/**
 * \sa device_mac_engine_entry()
 */
#define LPWAN_MAC_JOIN_AFTER_INITED_MIN     5
#define LPWAN_MAC_JOIN_AFTER_INITED_MAX     20

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_CONFIG_H_ */
