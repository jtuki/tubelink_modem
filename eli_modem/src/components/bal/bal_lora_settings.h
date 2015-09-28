/**
 * bal_lora_settings.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef BAL_LORA_SETTINGS_H_
#define BAL_LORA_SETTINGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "rf/rf_type.h"

/*!
 * SX1276 LoRa General parameters definition
 */
typedef struct sLoRaSettings
{
    rf_uint32 RFFrequency;
    rf_int8 Power;
    rf_uint8 SignalBw;                   // LORA [0: 7.8 kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                                        // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    rf_uint8 SpreadingFactor;            // LORA [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    rf_uint8 ErrorCoding;                // LORA [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    rf_bool CrcOn;                         // [0: OFF, 1: ON]
    rf_bool ImplicitHeaderOn;              // [0: OFF, 1: ON]
    rf_bool RxSingleOn;                    // [0: Continuous, 1 Single]
    rf_bool FreqHopOn;                     // [0: OFF, 1: ON]
    rf_uint8 HopPeriod;                  // Hops every frequency hopping period symbols
    rf_uint32 TxPacketTimeout;
    rf_uint32 RxPacketTimeout;
    rf_uint8 PayloadLength;
    rf_uint16 u16PreambleLength;
} tLoRaSettings;

extern tLoRaSettings LoRaSettings;

/**< Symbol timeout for single rx behavoir.
 * \ref sx1276's datasheet Page40 (symbol should be range within [4, 1023])
 * \ref sx1276's datasheet Page113 RegSymbTimeoutLsb */
#define LORA_SETTINGS_RX_SYMBOL_TIMEOUT     7

#ifdef __cplusplus
}
#endif

#endif /* BAL_LORA_SETTINGS_H_ */
