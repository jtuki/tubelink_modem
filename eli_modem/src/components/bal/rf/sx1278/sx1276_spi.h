#ifndef __SPI_H__
#define __SPI_H__
#include "sx1276_cfg.h"

#define USE_SX1276_RADIO
#ifdef USE_SX1276_RADIO

#include "stm8l15x.h"

extern void sx1276_spi_init(void);
extern void sx1276_spi_deinit(void);
extern void sx1276_spi_write(uint8_t a_u8Data);
extern uint8_t sx1276_spi_read(void);


/* end of USE_SX1276_RADIO */
#endif

/* end of __SPI_H__ */
#endif
