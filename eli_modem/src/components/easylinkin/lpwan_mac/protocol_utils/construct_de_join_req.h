/**
 * construct_de_join_req.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_DE_JOIN_REQ_H_
#define CONSTRUCT_DE_JOIN_REQ_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/common_defs.h"
#include "frame_defs/device_join_request.h"

void construct_device_join_req(struct device_join_request *join,
                               os_uint8 init_seq, enum _device_join_reason reason,
                               app_id_t app_id);

void update_device_join_req(struct device_join_request *join,
                            os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                            os_int16 bcn_rssi, os_int16 bcn_snr);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_DE_JOIN_REQ_H_ */
