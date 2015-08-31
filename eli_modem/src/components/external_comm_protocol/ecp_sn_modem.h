/**
 * ecp_sn_modem.h
 *
 *  The external communication protocol between sensor node modem and
 *  third-party developers' CPU .
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef ECP_SN_MODEM_H_
#define ECP_SN_MODEM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "frame_defs/common_defs.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

enum ecp_frame_type {
    ECP_FTYPE_CONTROL = 0,
    ECP_FTYPE_DATA,
    ECP_FTYPE_ACK,      /**< we don't care about ACK currently */
};

os_int8 ecp_sn_modem_dispatcher(os_uint8 *recv_frame_buf, os_uint16 len);

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ECP_SN_MODEM_H_ */
