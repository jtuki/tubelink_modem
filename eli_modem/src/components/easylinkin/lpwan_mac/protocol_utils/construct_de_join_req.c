/**
 * construct_de_join_req.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "construct_de_uplink_common.h"
#include "lib/hdk_utilities.h"
#include "construct_de_join_req.h"

void construct_device_join_req(struct device_join_request *join,
                               os_uint8 init_seq, enum _device_join_reason reason,
                               app_id_t app_id)
{
    join->app_id = os_hton_u32((os_uint32) app_id);
    join->join_reason = reason;
    join->init_seq_id = init_seq;
}

void update_device_join_req(struct device_join_request *join,
                            os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                            os_int16 bcn_rssi, os_int16 bcn_snr)
{
    update_device_uplink_common(& join->hdr, 0, 0,
                                bcn_seq_id, bcn_class_seq_id,
                                bcn_rssi, bcn_snr);
}
