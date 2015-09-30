/**
 * parse_gw_join_response.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parse_gw_join_response.h"
#include "frame_defs/gw_join_response.h"
#include "lib/hdk_utilities.h"

__LPWAN
os_int8 lpwan_parse_gw_join_confirmed (const void *join_confirmed,
                                       os_uint8 len,
                                       struct parsed_gw_join_confirmed *confirmed)
{
    if (len != sizeof(struct gw_join_confirmed))
        return -1;

    const struct gw_join_confirmed *confirm = join_confirmed;

    os_uint8 _info = get_bits(confirm->hdr, 7, 4);
    if (_info >= _join_confirmed_info_invalid) {
        return -1;
    } else {
        confirmed->info = (enum gw_join_confirmed_info) _info;
    }

    confirmed->init_seq_id = confirm->init_seq_id;
    confirmed->distributed_short_addr = \
            os_ntoh_u16((os_uint16) confirm->distributed_short_addr);

    return len;
}
