#ifndef _HTU21D_H
#define _HTU21D_H

#include "c_types.h"
#include "eagle_soc.h"
#include "driver/i2c.h"

#define HTU21D_ADDRESS     0x40

#define HTU21D_TEMP_MEASURE_HOLD  0xE3
#define HTU21D_HUMD_MEASURE_HOLD  0xE5
#define HTU21D_TEMP_MEASURE_NOHOLD  0xF3
#define HTU21D_HUMD_MEASURE_NOHOLD  0xF5
#define HTU21D_WRITE_USER_REG  0xE6
#define HTU21D_READ_USER_REG  0xE7
#define HTU21D_SOFT_RESET  0xFE

void htu21d_init();
void htu21d_reset();
bool htu21d_read_temp(float*);
bool htu21d_read_rh(float*);


#endif // _HTU21D
