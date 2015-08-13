/***************************************************************************************************
* @file name	app_config.c
* @data   		2015/04/14
* @auther   	chuanpengl
* @module   	configure module
* @brief 		configure
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_config.h"
#include "lib_util/crc/crc.h"
#include "lib/assert.h"


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define CFG_ASSERT          haddock_assert

#define CFG_MAC_BYTE0_DEFAULT        (0x02)
#define CFG_MAC_BYTE0_CONFIGURE      (0x00)

#define CFG_SN_DEFAULT              (1 << 7)
#define CFG_SN_CONFIGURE            (0 << 7)
#define CFG_SN_DEVICE_TYPE          (0 << 5)
#define CFG_SN_VERSION              (0x01 & 0x1F)

#define CFG_SN_BYTE0_DEFAULT        (CFG_SN_DEFAULT | CFG_SN_DEVICE_TYPE | CFG_SN_VERSION)
#define CFG_SN_BYTE0_CONFIGURE      (CFG_SN_CONFIGURE | CFG_SN_DEVICE_TYPE | CFG_SN_VERSION)

/***************************************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    cfgChar acMac[6];
    cfgChar acSn[8];
}cfgMacAndSn_t;
 

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      cfg_LoadMacAndSn__()
 *
 * @brief   load mac address and serial number
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void cfg_LoadMacAndSn__( void );


/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
static cfgMacAndSn_t gs_tMacAndSn;
static cfgChar *gs_pcUniqueId = (cfgChar *) 0x1FF80050;

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      cfg_Init()
 *
 * @brief   configure initial, read configure info from NVM when start or restart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_Init( void )
{
    cfg_LoadMacAndSn__();

} /* cfg_Init() */

/***************************************************************************************************
 * @fn      cfg_GetMac()
 *
 * @brief   get current mac address
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_GetMac( cfgChar *a_pcData, cfgUint8 a_u8Length )
{
    CFG_ASSERT((void*)0 != a_pcData);
    CFG_ASSERT(CFG_MAC_LENGTH == a_u8Length);
    
    (void) gs_tMacAndSn;
#if 0
    sprintf(a_pcData, "%02x%02x%02x%02x%02x%02x", \
        gs_tMacAndSn.acMac[0], \
            gs_tMacAndSn.acMac[1], \
                gs_tMacAndSn.acMac[2], \
                    gs_tMacAndSn.acMac[3], \
                        gs_tMacAndSn.acMac[4], \
                            gs_tMacAndSn.acMac[5]);
    
#endif

} /* cfg_GetMac() */

/***************************************************************************************************
 * @fn      cfg_GetSerialNumber()
 *
 * @brief   get current serial number
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_GetSerialNumber( cfgChar *a_pcData, cfgUint8 a_u8Length )
{
    CFG_ASSERT((void*)0 != a_pcData);
    CFG_ASSERT(CFG_SN_LENGTH == a_u8Length);
#if 0  
    sprintf(a_pcData, "%02x%02x%02x%02x%02x%02x%02x%02x", \
        gs_tMacAndSn.acSn[0], \
            gs_tMacAndSn.acSn[1], \
                gs_tMacAndSn.acSn[2], \
                    gs_tMacAndSn.acSn[3], \
                        gs_tMacAndSn.acSn[4], \
                            gs_tMacAndSn.acSn[5], \
                                gs_tMacAndSn.acSn[6], \
                                    gs_tMacAndSn.acSn[7]);
#endif
    

} /* cfg_GetSerialNumber() */
/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      cfg_LoadMacAndSn__()
 *
 * @brief   load mac address and serial number
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_LoadMacAndSn__( void )
{
    cfgUint8 u8Crc = 0;
    
    if(0)   /* load from NVM failed */
    {
    }
    else
    {
        u8Crc = CRC_Calc(POLY_CRC8_CCITT, gs_pcUniqueId, 12);
        gs_tMacAndSn.acMac[0] = CFG_MAC_BYTE0_DEFAULT;
        gs_tMacAndSn.acMac[1] = gs_pcUniqueId[3];
        gs_tMacAndSn.acMac[2] = gs_pcUniqueId[2];
        gs_tMacAndSn.acMac[3] = gs_pcUniqueId[1];
        gs_tMacAndSn.acMac[4] = gs_pcUniqueId[0];
        gs_tMacAndSn.acMac[5] = u8Crc;
        
        gs_tMacAndSn.acSn[0] = CFG_SN_BYTE0_DEFAULT;
        gs_tMacAndSn.acSn[1] = gs_pcUniqueId[5];
        gs_tMacAndSn.acSn[2] = gs_pcUniqueId[4];
        gs_tMacAndSn.acSn[3] = gs_pcUniqueId[3];
        gs_tMacAndSn.acSn[4] = gs_pcUniqueId[2];
        gs_tMacAndSn.acSn[5] = gs_pcUniqueId[1];
        gs_tMacAndSn.acSn[6] = gs_pcUniqueId[0];
        gs_tMacAndSn.acSn[7] = u8Crc;
        
        
    }
    

} /* cfg_LoadMacAndSn__() */ 

 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
