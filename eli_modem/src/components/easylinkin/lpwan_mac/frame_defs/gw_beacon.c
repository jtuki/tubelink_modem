/**
 * gw_beacon.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "gw_beacon.h"

/** The length of each beacon period (unit: seconds).
 * \sa enum _beacon_period */
const os_uint8 gl_beacon_period_length_list[4] = {2, 4, 8, 16};

/** We segment each beacon period into 128 sections/slots.
 * Below are the section length for different beacon period length. (uint: us)
 * \sa beacon_period_section_ratio_t
 * \sa gl_beacon_period_length_list */
const os_uint32 gl_beacon_section_length_us[4] = {15625, 31250, 62500, 125000};
