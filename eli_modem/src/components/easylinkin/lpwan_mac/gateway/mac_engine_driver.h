/**
 * mac_engine_driver.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef MAC_ENGINE_DRIVER_H_
#define MAC_ENGINE_DRIVER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "lpwan_utils.h"

#include "kernel/process.h"

#include "external_comm_protocol/ecp_gw_modem.h"

extern os_pid_t gl_gateway_mac_engine_driver_pid;

extern void gateway_mac_engine_driver_init(os_uint8 priority);

#define SIGNAL_MAC_DRIVER_START_ENGINE                      BV(0)
#define SIGNAL_MAC_DRIVER_STOP_ENGINE                       BV(1)
#define SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE                 BV(2)
#define SIGNAL_MAC_DRIVER_CONFIG_BEACON_CLASSES_NUM_UPDATE  BV(3)
#define SIGNAL_MAC_DRIVER_CONFIG_MODEM_CHANNEL_UPDATE       BV(4)

void mac_driver_config_set(enum ecp_gw_c2m_control_code item, void *config_value);
void mac_driver_config_get(enum ecp_gw_c2m_control_code item, void *config_value);
const modem_uuid_t *mac_driver_get_modem_uuid(void);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_DRIVER_H_ */
