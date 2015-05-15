/**
 * lpwan_config.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lpwan_config.h"

const radio_channel_t lpwan_radio_channels_list[RADIO_CHANNELS_MAX_NUM] = {
        470*1000000, 471*1000000, 472*1000000, // todo radio channel list
};

const os_int8 lpwan_radio_tx_power_list[RADIO_TX_POWER_LEVELS_NUM] = {
        0, 7, 13, 20 // todo radio tx power list
};
