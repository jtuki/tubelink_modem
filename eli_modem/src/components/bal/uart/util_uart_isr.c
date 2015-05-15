#include "util_uart.h"

/*!what should this file do?
 *receive data form uart, and store in buffer
 *send data(received from lora if) by uart
 */



/*! IF between uart int and buffer 
*/


/*! receive char by int
 */
void util_uart_rxchar_write(uint8_t a_u8Data)
{
}



/*! send char by int 
 *
 */
uint8_t util_uart_txchar_read(void)
{
  return 0;
}



/*! IF between buffer and application
*/

/*!uart_buffer_read
*/
void util_uart_buffer_read()
{}

/*!uart buffer write*/
void util_uart_buffer_write()
{
}