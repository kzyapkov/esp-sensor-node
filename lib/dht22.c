
/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

#include "ets_sys.h"
#include "osapi.h"
#include "espmissingincludes.h"
#include "c_types.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"
#include "gpio.h"

#define MAXTIMINGS 10000
#define BREAKTIME 20


// #define DHT_PIN_MUX     PERIPHS_IO_MUX_GPIO2_U
// #define DHT_PIN_FUNC    FUNC_GPIO2
// #define DHT_PIN         2

#define DHT_PIN_MUX     PERIPHS_IO_MUX_U0TXD_U
#define DHT_PIN_FUNC    FUNC_GPIO1
#define DHT_PIN         1

int8_t ICACHE_FLASH_ATTR
DHTread(float* temperature, float* humidity)
{
    int counter = 0;
    int laststate = 1;
    int i = 0;
    int j = 0;
    int checksum = 0;
    int data[7];

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;

    // GPIO_OUTPUT_SET(DHT_PIN, 1);
    // os_delay_us(250000);
    GPIO_OUTPUT_SET(DHT_PIN, 0);
    os_delay_us(20000);
    GPIO_OUTPUT_SET(DHT_PIN, 1);
    os_delay_us(40);
    GPIO_DIS_OUTPUT(DHT_PIN);
    PIN_PULLUP_EN(DHT_PIN_MUX);

    // wait for pin to drop?
    while (GPIO_INPUT_GET(DHT_PIN) == 1 && i<100000) {
          os_delay_us(1);
          i++;
    }

    if(i==100000) {
        return -5;
    }

    // read data!
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;
        while (GPIO_INPUT_GET(DHT_PIN) == laststate) {
            counter++;
            os_delay_us(1);
            if (counter == 1000) {
                break;
            }
        }
        laststate = GPIO_INPUT_GET(DHT_PIN);
        if (counter == 1000) break;

        //bits[bitidx++] = counter;

        if ((i>3) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            data[j/8] <<= 1;
            if (counter > BREAKTIME)
                data[j/8] |= 1;
            j++;
        }
    }

    float temp_p, hum_p;
    if (j >= 39) {
        checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
        if (data[4] == checksum) {
            // yay! checksum is valid

            hum_p = data[0] * 256 + data[1];
            hum_p /= 10;

            temp_p = (data[2] & 0x7F)* 256 + data[3];
            temp_p /= 10.0;
            if (data[2] & 0x80)
                temp_p *= -1;
			os_printf("Temp =  %d *C, Hum = %d %%\n", (int)(temp_p*100), (int)(hum_p*100));
			*temperature = temp_p;
			*humidity = hum_p;
        }
    }
	return 1;
}

void DHTInit() {
    //Set GPIO2 to output mode for DHT22
    PIN_FUNC_SELECT(DHT_PIN_MUX, DHT_PIN_FUNC);
    GPIO_DIS_OUTPUT(DHT_PIN);
    PIN_PULLUP_EN(DHT_PIN_MUX);
}
