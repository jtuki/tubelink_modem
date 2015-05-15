#ifndef __SX1276_CFG_H__
#define __SX1276_CFG_H__

/*!
 * radio enable or not
 * if radio enable, define USE_SX1276_RADIO
 * else not define USE_SX1276_RADIO
 */
#define USE_SX1276_RADIO

/*!
 * platform define
 */
#define PLATFORM_SX1276_DISCOVERY (0)


/*!
 * platform select
 */
#define PLATFORM PLATFORM_SX1276_DISCOVERY

/*!
 * Module choice. There are three existing module with the SX1276.
 * Please set the connected module to the value 1 and set the others to 0
 */
#ifdef USE_SX1276_RADIO
#define MODULE_SX1276RF1IAS                         1
#define MODULE_SX1276RF1JAS                         0
#define MODULE_SX1276RF1KAS                         0
#endif


/*!
 * LoRa or FSK
 */
#define LORA                                    (1)


#if(PLATFORM == PLATFORM_SX1276_DISCOVERY)
#include "stm8l15x.h"
#endif


#define TICK_RATE_MS( ms )                          ( ms )



#define false               FALSE
#define true                TRUE



#endif
