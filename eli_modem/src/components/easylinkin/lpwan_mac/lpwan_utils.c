/**
 * lpwan_utils.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lpwan_utils.h"
#include "lpwan_config.h"

/**
 * \return Positive value if seq1>seq2, else return negative value.
 *      the returned value's absolute value is the delta.
 */
os_int16 calc_bcn_seq_delta(os_int8 _seq1, os_int8 _seq2)
{
    os_int16 seq1 = (os_int16) _seq1;
    os_int16 seq2 = (os_int16) _seq2;

    if (seq1 > 0 && seq2 > 0) {
        if (seq1 == seq2) {
            return 0;
        } else if (seq1 > seq2 && (seq1 - seq2) <= (BEACON_MAX_SEQ_NUM / 2)) {
            return seq1 - seq2;
        } else if ((seq1 - seq2) > (BEACON_MAX_SEQ_NUM / 2)) {
            return -((BEACON_MAX_SEQ_NUM - seq1) + 1 + seq2);
        } else { // seq1 < seq2
            return (0 - calc_bcn_seq_delta(_seq2, _seq1));
        }
    } else { // both less than 0 or have different sign.
        return seq1 - seq2;
    }
}

short_modem_uuid_t short_modem_uuid(const modem_uuid_t *uuid)
{
    // for stm32L0x1 Cortex-M0+ series
    os_uint8 x0 = uuid->addr[11];
    os_uint8 x1 = uuid->addr[10];
    os_uint8 y0 = uuid->addr[9];
    os_uint8 y1 = uuid->addr[8];
    os_uint8 wafer_number = uuid->addr[7];

    os_uint16 higher = (os_uint16) (((x0 + x1) ^ (wafer_number + 91)) << 8);
    os_uint16 lower  = (os_uint16) ((y0 + y1) ^ (wafer_number + 91));
    return (short_modem_uuid_t) (higher + lower);
}

void mcu_read_unique_id(modem_uuid_t *uuid)
{
    /**
     * For stm32L0x1 Cortex-M0+ series.
     * \note There is a hole in the unique ID. Refer to the reference manual.
     * \ref http://tinyurl.com/ptt4dfu
     */

    os_uint8 *_mcu_unique_id_first_8B = (os_uint8 *) 0x1FF80050;
    os_uint8 *_mcu_unique_id_last_4B  = (os_uint8 *) 0x1FF80050 + 0x14;

    uuid->addr[0] = _mcu_unique_id_first_8B[0];
    uuid->addr[1] = _mcu_unique_id_first_8B[1];
    uuid->addr[2] = _mcu_unique_id_first_8B[2];
    uuid->addr[3] = _mcu_unique_id_first_8B[3];
    uuid->addr[4] = _mcu_unique_id_first_8B[4];
    uuid->addr[5] = _mcu_unique_id_first_8B[5];
    uuid->addr[6] = _mcu_unique_id_first_8B[6];
    uuid->addr[7] = _mcu_unique_id_first_8B[7];

    uuid->addr[8]  = _mcu_unique_id_last_4B[0];
    uuid->addr[9]  = _mcu_unique_id_last_4B[1];
    uuid->addr[10] = _mcu_unique_id_last_4B[2];
    uuid->addr[11] = _mcu_unique_id_last_4B[3];
}

os_uint32 mcu_generate_seed_from_uuid(const modem_uuid_t *uuid)
{
    return construct_u32_4(91 + uuid->addr[11],
                           91 + uuid->addr[10],
                           91 + uuid->addr[9],
                           91 + uuid->addr[8]);
}

os_boolean lpwan_uuid_is_equal(const modem_uuid_t *self, const modem_uuid_t *uuid)
{
    for (size_t i=0; i<12; i++) {
        if (self->addr[i] != uuid->addr[i])
            return OS_FALSE;
    }
    return OS_TRUE;
}

os_boolean lpwan_uuid_is_broadcast(const modem_uuid_t *uuid)
{
    for (size_t i=0; i<12; i++) {
        if (uuid->addr[i] != 0xFF)
            return OS_FALSE;
    }
    return OS_TRUE;
}
