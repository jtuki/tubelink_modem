/**
 * bal_lora_settings.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "bal_lora_settings.h"

// Default settings - (jt: this is the Lora configuration what we are using)
tLoRaSettings LoRaSettings =
{
    423000000,        // RFFrequency
    20,               // Power
    8,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    7,                // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    3,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    rf_true,           // CrcOn [0: OFF, 1: ON]
    rf_false,          // ImplicitHeaderOn [0: OFF, 1: ON]
    rf_true,           // RxSingleOn [0: Continuous, 1 Single]
    rf_false,          // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    2000,             // TxPacketTimeout
    /* here, if in cad mode, rx timeout value may be need greater then sleep time */
    20,               // RxPacketTimeout
    128,              // PayloadLength (used for implicit header mode)
    32,               /* preamble length */
};
