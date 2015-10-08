/**
 * lpwan_radio.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "haddock_types.h"
#include "kernel/hdk_memory.h"
#include "kernel/ipc.h"
#include "kernel/process.h"

#include "lib/assert.h"
#include "simple_log.h"

#include "radio_config.h"
#include "lpwan_radio.h"
#include "radio_controller/radio_controller.h"

#include "sx1278/rf_manager.h"
#include "sx1278/sx1278.h"

/**
 * \note The first byte contain the length of the rx frame length.
 */
os_uint8 lpwan_radio_rx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];

/**
 * The begin and end time of the last rx frame.
 */
static struct time tmp_rx_time;
static struct time rx_begin_time;
static struct time rx_end_time;

/**
 * The begin and end time of the last tx frame.
 */
static struct time tx_begin_time;
static struct time tx_end_time;

/***************************************************************************************************
 * @fn      lpwan_radio_event__()
 *
 * @brief   lpwan radio events
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void lpwan_radio_event__(RF_EVT_e a_eEvt);

static os_pid_t _lpwan_radio_controller_proc_id = PROCESS_ID_RESERVED;

void lpwan_radio_register_radio_controller_pid(os_pid_t pid)
{
    _lpwan_radio_controller_proc_id = pid;
}

/**
 * Try to send a frame through radio interface.
 * @return 0 if ok, negative value if failed.
 */
os_int8 lpwan_radio_tx(const os_uint8 frame[], os_uint16 len)
{
    haddock_assert(Rf_GetCurState() == RF_STANDBY);
    // print_log(LOG_INFO, "RTx");
    if (len > LPWAN_RADIO_TX_BUFFER_MAX_LEN)
        return LPWAN_RADIO_ERR_TX_LEN_INVALID;
    
    haddock_get_time_tick_now_cached(& tx_begin_time);
    Rf_Send((rf_char*)frame, len);
    return 0;
}

/**
 * Send a command to radio interface to start radio reception.
 * @return 0 if ok, negative value if failed.
 */
os_int8 lpwan_radio_start_rx(void)
{
    haddock_assert(Rf_GetCurState() == RF_STANDBY);
    Rf_ReceiveStart();
    haddock_get_time_tick_now_cached(& tmp_rx_time);
    return 0;
}

os_int8 lpwan_radio_stop_rx(void)
{
    Rf_Stop();
    return 0;
}

/**
 * If the radio has received a frame (contained in @lpwan_radio_rx_buffer), copy 
 * the frame into @buffer (with length @len).
 * @return 0 if ok, negative value if failed.
 */
os_int8 lpwan_radio_read(os_uint8 buffer[], os_uint16 len)
{
    // print_log(LOG_INFO, "RRead");
    lpwan_radio_rx_buffer[0] = (os_uint8)Rf_Get((rf_char*)&lpwan_radio_rx_buffer[1], LPWAN_RADIO_RX_BUFFER_MAX_LEN);
    if (1+lpwan_radio_rx_buffer[0] > len) {
        buffer[0] = 0;
        return LPWAN_RADIO_ERR_READ_BUFFER_LEN_INVALID;
    }
    else {
        haddock_memcpy(buffer, lpwan_radio_rx_buffer, 1+lpwan_radio_rx_buffer[0]);
        return 0;
    }
}

/**
 * Call this function before the next frame has fully received.
 */
os_int16 lpwan_radio_get_snr(void)
{
    return (os_int16) Rf_GetPacketSnr();
}

/**
 * Call this function before the next frame has fully received.
 */
os_int16 lpwan_radio_get_rssi(void)
{
    return (os_int16) Rf_GetPacketRssi();
}

struct lpwan_last_rx_frame_rssi_snr lpwan_radio_get_last_rx_rssi_snr(void)
{
    struct lpwan_last_rx_frame_rssi_snr signal_strength;
    signal_strength.rssi = lpwan_radio_get_rssi();
    signal_strength.snr  = lpwan_radio_get_snr();
    return signal_strength;
}

void lpwan_radio_get_last_rx_frame_time(struct lpwan_last_rx_frame_time *t)
{
    t->tx = rx_begin_time;
    t->rx = rx_end_time;
}

void lpwan_radio_get_last_tx_frame_time(struct lpwan_last_tx_frame_time *t)
{
    t->tx_begin = tx_begin_time;
    t->tx_end = tx_end_time;
}

/**
 * Set the base frequency.
 */
void lpwan_radio_change_base_frequency(os_uint32 freq)
{
    SX1276LoRaSetRFBaseFrequency(freq);
}

/***************************************************************************************************
 * @fn      lpwan_radio_init()
 *
 * @brief   init lpwan radio
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void lpwan_radio_init( void )
{
    /* init rf module */
    Rf_Init(lpwan_radio_event__);
    
} /* appLora_Init() */


/***************************************************************************************************
 * @fn      lpwan_radio_routine()
 *
 * @brief   lpwan radio routine
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void lpwan_radio_routine( void )
{
    Rf_Routine(0);
    
} /* lpwan_radio_routine() */

/***************************************************************************************************
 * @fn      lpwan_radio_event__()
 *
 * @brief   lpwan radio events
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void lpwan_radio_event__(RF_EVT_e a_eEvt)
{
    haddock_assert(_lpwan_radio_controller_proc_id != PROCESS_ID_RESERVED);

    switch( a_eEvt )
    {
    case RF_EVT_IDLE:
        //Rf_ReceiveStart();
        break;
    case RF_EVT_RX_OK:
        os_ipc_set_signal(_lpwan_radio_controller_proc_id, SIGNAL_LPWAN_RADIO_RX_OK);
        rx_begin_time = tmp_rx_time;
        haddock_get_time_tick_now_cached(& rx_end_time);
        break;
    case RF_EVT_RX_TMO:
        os_ipc_set_signal(_lpwan_radio_controller_proc_id, SIGNAL_LPWAN_RADIO_RX_TIMEOUT);
        break;
    case RF_EVT_RX_CRCERR:
        os_ipc_set_signal(_lpwan_radio_controller_proc_id, SIGNAL_LPWAN_RADIO_RX_CRC_ERROR);
        break;
    case RF_EVT_TX_OK:
        os_ipc_set_signal(_lpwan_radio_controller_proc_id, SIGNAL_LPWAN_RADIO_TX_OK);
        haddock_get_time_tick_now_cached(& tx_end_time);
        break;
    case RF_EVT_TX_TMO:
        os_ipc_set_signal(_lpwan_radio_controller_proc_id, SIGNAL_LPWAN_RADIO_TX_TIMEOUT);
        /** \note
         * jt - Even if tx timeout, we also record the tx_end_time */
        haddock_get_time_tick_now_cached(& tx_end_time);
        break;
    default:
        break;
    }
}   /* lpwan_radio_event__() */


/***************************************************************************************************
 * @fn      lpwan_radio_sleep()
 *
 * @brief   lpwan radio sleep
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void lpwan_radio_sleep( void )
{
}   /* lpwan_radio_sleep() */



/***************************************************************************************************
 * @fn      lpwan_radio_wakeup()
 *
 * @brief   lpwan radio wakeup
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void lpwan_radio_wakeup( void )
{
}   /* lpwan_radio_sleep() */
