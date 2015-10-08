/**
 * lpwan_utils.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef LPWAN_UTILS_H_
#define LPWAN_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lib/hdk_utilities.h"

#include "lpwan_types.h"
#include "frame_defs/common_defs.h"

short_modem_uuid_t short_modem_uuid(const modem_uuid_t *uuid);

void mcu_read_unique_id(modem_uuid_t *uuid);
os_uint32 mcu_generate_seed_from_uuid(const modem_uuid_t *uuid);

os_boolean lpwan_uuid_is_equal(const modem_uuid_t *self, const modem_uuid_t *uuid);
os_boolean lpwan_uuid_is_broadcast(const modem_uuid_t *uuid);

os_int16 calc_bcn_seq_delta(os_int8 _seq1, os_int8 _seq2);

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_UTILS_H_ */
