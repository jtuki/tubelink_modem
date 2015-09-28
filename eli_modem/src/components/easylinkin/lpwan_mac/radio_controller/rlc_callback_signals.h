/**
 * rlc_callback_signals.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef RLC_CALLBACK_SIGNALS_H_
#define RLC_CALLBACK_SIGNALS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * RLC (radio controller layer)'s callback signals (sent to registered caller.)
 * \sa rlc_register_caller()
 */
#define SIGNAL_RLC_TX_OK                    BV(30)
#define SIGNAL_RLC_TX_TIMEOUT               BV(29)
#define SIGNAL_RLC_TX_CCA_FAILED            BV(28)
#define SIGNAL_RLC_TX_CCA_CRC_FAIL          BV(27)
#define SIGNAL_RLC_RX_DURATION_TIMEOUT      BV(26)
#define SIGNAL_RLC_RX_OK                    BV(25)

#ifdef __cplusplus
}
#endif

#endif /* RLC_CALLBACK_SIGNALS_H_ */
