/**
 * parse_frame_hdr.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSE_FRAME_HDR_H_
#define PARSE_FRAME_HDR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "parsed_common_defs.h"

/**< \sa struct frame_header */
struct parsed_frame_hdr_info {
    enum _device_type frame_origin_type;
    union {
        struct {
            enum frame_type_gw frame_type;
            os_boolean is_end_of_section;
            enum _beacon_period_section end_of_beacon_section;
            os_boolean is_multicast_dest;
        } gw;
        struct {
            enum frame_type_end_device frame_type;
            os_boolean is_mobile;
            os_uint8 tx_power_level;
        } de;
    } info;
    struct lpwan_addr dest;
    struct lpwan_addr src;
};

__LPWAN os_int8 lpwan_parse_frame_header (const struct frame_header *hdr, os_uint8 len,
                                       struct parsed_frame_hdr_info *info);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_FRAME_HDR_H_ */
