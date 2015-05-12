#include "espmissingincludes.h"
#include "osapi.h"
#include "HTU21D.h"

// The following function is from
// HTU21D Humidity Sensor Example Code
// By: Nathan Seidle
// https://github.com/sparkfun/HTU21D0sf _Breakout
#define byte uint8_t        // compat
//Give this function the 2 byte message (measurement) and the check_value byte from the HTU21D
//If it returns 0, then the transmission was good
//If it returns something other than 0, then the communication was corrupted
//From: http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html
//POLYNOMIAL = 0x0131 = x^8 + x^5 + x^4 + 1 : http://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks
#define SHIFTED_DIVISOR 0x988000 //This is the 0x0131 polynomial shifted to farthest left of three bytes

uint8_t check_crc(uint16_t message_from_sensor, uint8_t check_value_from_sensor)
{
  //Test cases from datasheet:
  //message = 0xDC, checkvalue is 0x79
  //message = 0x683A, checkvalue is 0x7C
  //message = 0x4E85, checkvalue is 0x6B

  //Pad with 8 bits because we have to add in the check value
  uint32_t remainder = (uint32_t)message_from_sensor << 8;
  remainder |= check_value_from_sensor; //Add on the check value

  uint32_t divsor = (uint32_t)SHIFTED_DIVISOR;

  int i;
  // Operate on only 16 positions of max 24.
  // The remaining 8 are our remainder and should be zero when we're done.
  for (i = 0 ; i < 16 ; i++)
  {
    //Serial.print("remainder: ");
    //Serial.println(remainder, BIN);
    //Serial.print("divsor:    ");
    //Serial.println(divsor, BIN);
    //Serial.println();

    if( remainder & (uint32_t)1<<(23 - i) ) //Check if there is a one in the left position
      remainder ^= divsor;

    divsor >>= 1; //Rotate the divsor max 16 times so that we have 8 bits left of a remainder
  }

  return (byte)remainder;
}

void htu21d_init() {
    htu21d_reset(); // "... takes less than 15ms."
}

void htu21d_reset() {
    i2c_start_write(HTU21D_ADDRESS);
    i2c_writeByte(HTU21D_SOFT_RESET);
    i2c_stop();
}

int8_t htu21d_read_raw(uint8_t cmd, uint16_t* raw) {
    int8_t res = 0;
    uint8_t crc;

    i2c_start_write(HTU21D_ADDRESS);
    if (!i2c_check_ack()) {
        // os_printf("htu21d_read_raw: no ack after address write\n");
        res = -1;
        goto stop_and_fail;
    }

    i2c_writeByte(cmd);
    if (!i2c_check_ack()) {
        // os_printf("htu21d_read_raw: no ack after TEMP_MEASURE_NOHOLD write\n");
        res = -2;
        goto stop_and_fail;
    }
    i2c_stop();

    os_delay_us(50000);

    i2c_start_read(HTU21D_ADDRESS);
    if (!i2c_check_ack()) {
        // os_printf("htu21d_read_raw: no ack on reading start\n");
        res = -2;
        goto stop_and_fail;
    }

    // read 3 bytes, stop i2c
    *raw = 0x0000;
    *raw |= (i2c_readByte() << 8);
    i2c_send_ack(1);
    *raw |= i2c_readByte();
    i2c_send_ack(1);
    crc = i2c_readByte();
    i2c_send_ack(0);
    i2c_stop();

    // os_printf("htu21d_read_raw: @0x%02x: 0x%04x, 0x%02x\n", cmd, *raw, crc);
    uint8_t check = check_crc(*raw, crc);
    if (check != 0x00) {
        // os_printf("htu21d_read_raw: checksum failed, remainder=0x%04x\n");
        return -3;
    }

    *raw &= 0xFFFC;
    return 0;

    stop_and_fail: {
        // os_printf("htu21d_read: faied.\n");
        i2c_stop();
        return (res < 0) ? res : -5;
    }
}

bool htu21d_read_temp(float* temp) {
    uint16_t raw_temp;

    if (htu21d_read_raw(HTU21D_TEMP_MEASURE_NOHOLD, &raw_temp) < 0) {
        return false;
    }

	*temp = raw_temp / (float)65536;
	*temp = -46.85 + (175.72 * (*temp));
	return true;
}

bool htu21d_read_rh(float* rh) {
    uint16_t raw_rh;
    if (htu21d_read_raw(HTU21D_HUMD_MEASURE_NOHOLD, &raw_rh) < 0) {
        return false;
    }

    *rh = raw_rh / (float)65536;
    *rh = -6 + (125 * (*rh));
    return true;
}
