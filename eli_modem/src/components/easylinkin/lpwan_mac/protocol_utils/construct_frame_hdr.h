/**
 * construct_frame_hdr.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_FRAME_HDR_H_
#define CONSTRUCT_FRAME_HDR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/frame_defs.h"
#include "parsed_common_defs.h"

/**
 * \sa struct frame_header
 */

os_int8 construct_device_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                       enum frame_type_end_device type,
                                       struct lpwan_addr *src,
                                       struct lpwan_addr *dest,
                                       os_boolean is_mobile,
                                       os_uint8 tx_power_level);

os_int8 construct_gateway_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                        enum frame_type_gw type,
                                        struct lpwan_addr *src,
                                        struct lpwan_addr *dest,
                                        os_boolean is_multicast_dest,
                                        os_boolean is_end_of_section,
                                        enum _beacon_period_section end_of_beacon_section);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_FRAME_HDR_H_ */
