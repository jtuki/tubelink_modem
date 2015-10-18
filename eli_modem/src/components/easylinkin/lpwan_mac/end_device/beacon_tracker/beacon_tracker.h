/**
 * beacon_tracker.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef BEACON_TRACKER_H_
#define BEACON_TRACKER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel/process.h"
#include "haddock_types.h"

#include "protocol_utils/parse_beacon.h"

enum beacon_tracker_states {
    BCN_TRACKER_INITED,
    /**< search possible beacon */
    BCN_TRACKER_SEARCH_BEACON,
    /**< beacon found (during search beacon period).
     * If MAC layer regards the beacon as valid, enter beacon tracking loop.
     * \sa SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING*/
    BCN_TRACKER_BEACON_FOUND,
    /**< beacon tracking */
    BCN_TRACKER_BCN_TRACKING,
    /**< missed beacon during tracking, search beacon again. */
    BCN_TRACKER_BCN_TRACKING_SEARCH,
};

struct beacon_tracker_info_base {
    os_int8 expected_beacon_seq_id;
    os_uint8 expected_beacon_classes_num;   /**< still the same classes number? */
    os_uint8 expected_class_seq_id;

    /**< This flag tells the beacon tracker that if a beacon has been tracked,
     * whether or not it should check the packed ACK. */
    os_boolean is_check_packed_ack;

    /**< If @is_check_packed_ack == TRUE && @is_check_join_ack == TRUE,
     * the end-device is waiting for the JOIN_CONFIRM. */
    os_boolean is_check_join_ack;
};

/*---------------------------------------------------------------------------*/
/**< signals @{ */

#define SIGNAL_BTRACKER_INIT_FINISHED               BV(1)
#define SIGNAL_BTRACKER_SEARCH_TIMEOUT              BV(2)
#define SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING       BV(3)
#define SIGNAL_BTRACKER_TRACK_BEACON                BV(4)
#define SIGNAL_BTRACKER_TRACK_TIMEOUT               BV(5)
#define SIGNAL_BTRACKER_BEGIN_TRACK_SEARCH          BV(6)
#define SIGNAL_BTRACKER_TRACK_SEARCH_TIMEOUT        BV(7)

#include "radio_controller/rlc_callback_signals.h"

/**< @} */
/*---------------------------------------------------------------------------*/
/**< Timeing settings @{ */

#define DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS   100
#define DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS                      \
            (DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS +            \
             (gl_beacon_section_length_us[LPWAN_BEACON_PERIOD]  \
              * LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS / 1000))

/**< @} */
/*---------------------------------------------------------------------------*/

extern os_pid_t gl_btracker_pid;

void btracker_init(os_int8 priority);
void btracker_register_mac_engine(os_pid_t mac_engine_pid);

void btracker_search_beacon(os_uint16 duration);

void btracker_reset(void);

os_uint16 btracker_calc_beacon_span(void);
os_uint16 btracker_calc_beacon_rx_till_now_delta(void);
os_uint16 btracker_calc_beacon_tx_till_now_delta(void);
os_uint16 btracker_calc_beacon_span_remains(void);

const struct parsed_frame_hdr_info *btracker_get_parsed_frame_hdr_info(void);
const struct parsed_beacon_info *btracker_get_parsed_beacon_info(void);
const struct parsed_beacon_packed_ack_to_me *btracker_get_parsed_packed_ack(void);
const struct beacon_tracker_info_base *btracker_get_info_base(void);
const struct lpwan_last_rx_frame_time *btacker_get_last_bcn_time(void);
const struct lpwan_last_rx_frame_rssi_snr *btracker_get_last_bcn_signal_strength(void);

#ifdef __cplusplus
}
#endif

#endif /* BEACON_TRACKER_H_ */
