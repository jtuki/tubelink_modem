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


#define LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM    5

#define LPWAN_DEVICE_MAC_UPLINK_MTU         40

/**
 * \sa device_mac_engine_entry()
 */
#define LPWAN_MAC_JOIN_AFTER_INITED_MIN     1
#define LPWAN_MAC_JOIN_AFTER_INITED_MAX     10

/** If search beacon timeout, delay 300s to search again. */
#define LPWAN_MAC_SEARCH_BCN_AGAIN_MS       (300*1000)

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_CONFIG_H_ */
