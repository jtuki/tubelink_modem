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
                                       const struct lpwan_addr *src,
                                       const struct lpwan_addr *dest,
                                       os_boolean is_mobile,
                                       os_uint8 tx_power_level);

void update_device_frame_header_addr(struct frame_header *hdr, os_uint8 hdr_len,
                                     enum frame_type_end_device type,
                                     const struct lpwan_addr *src,
                                     const struct lpwan_addr *dest);

os_int8 construct_gateway_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                        enum frame_type_gw type,
                                        const struct lpwan_addr *src,
                                        const struct lpwan_addr *dest,
                                        os_boolean is_multicast_dest);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_FRAME_HDR_H_ */
