/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.h
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       May 6 2013
 * \author     Gregory Cristian
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#ifndef __SX1276_HAL_H__
#define __SX1276_HAL_H__

#include "sx1278_cfg.h"

#if (BAL_USE_SX1278 == BAL_MODULE_ON)

#include "stm32l0xx_hal.h"

/*!
 * DIO state read functions mapping
 */
#define DIO0                                        sx1278_DioReadDio0( )
#define DIO1                                        sx1278_DioReadDio1( )
#define DIO2                                        sx1278_DioReadDio2( )
#define DIO3                                        sx1278_DioReadDio3( )
#define DIO4                                        sx1278_DioReadDio4( )
#define DIO5                                        sx1278_DioReadDio5( )

/* RXTX pin control see errata note */
#define RXTX( _txEnable )                           sx1278_DioSetDioCtrl( _txEnable );


/***************************************************************************************************
 * @fn      sx1278_DioInit()
 *
 * @brief   sx1278 dio init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioInit(void);

/***************************************************************************************************
 * @fn      sx1278_DioDeInit()
 *
 * @brief   sx1278 dio de init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioDeInit(void);

/***************************************************************************************************
 * @fn      sx1276_Dio2EnableInt()
 *
 * @brief   sx1278 dio enable interrupt dio 2
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1276_Dio2EnableInt( rfBool a_bEnable );

/***************************************************************************************************
 * @fn      sx1278_DioReset()
 *
 * @brief   sx1278 dio reset
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioReset( rfBool a_bReset );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio0()
 *
 * @brief   sx1278 dio read dio 0
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio0( void );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio1()
 *
 * @brief   sx1278 dio read dio 1
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio1( void );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio2()
 *
 * @brief   sx1278 dio read dio 2
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio2( void );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio3()
 *
 * @brief   sx1278 dio read dio 3
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio3( void );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio4()
 *
 * @brief   sx1278 dio read dio 4
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio4( void );

/***************************************************************************************************
 * @fn      sx1278_DioReadDio5()
 *
 * @brief   sx1278 dio read dio 5
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint8 sx1278_DioReadDio5( void );


/***************************************************************************************************
 * @fn      sx1278_DioSetDioCtrl()
 *
 * @brief   sx1278 dio read dio ctrl
 *
 * @author  chuanpengl
 *
 * @param   a_bTxEnable  - rfTrue, tx mode
 *                       - rfFalse, rx mode
 *
 * @return  none
 */
extern void sx1278_DioSetDioCtrl( rfBool a_bTxEnable );

#endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */

#endif /*__SX1278_DIO_H__ */
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150707
*   context: here write modified history
*
***************************************************************************************************/

