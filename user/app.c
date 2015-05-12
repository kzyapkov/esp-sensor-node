#define USE_OPTIMIZE_PRINTF
#include "espmissingincludes.h"
#include "ets_sys.h"
#include "c_types.h"
#include "os_type.h"
#include "osapi.h"
#include "eagle_soc.h"
#include "user_interface.h"
#include "gpio.h"
#include "httpclient.h"
#include "driver/i2c.h"

#include "HTU21D.h"
#include "ds18b20.h"
#include "dht22.h"

#define TICK_PAUSE          2000
#define SAMPLE_COUNT        5

const char ssid[] = "ze netvork";
const char password[] = "putkeymyknee";

const char temp_feed_url[] = "http://api.thingspeak.com/update?api_key=18VVA67LTIFRVM0D";

struct station_config stationConf;

// all sensors given equal chance
double htu_temp_accum;
double htu_rh_accum;
double dht_temp_accum;
double dht_rh_accum;
double ds_temp_accum;
uint8_t accum_count;



static int ICACHE_FLASH_ATTR
print_float(char * target, float val) {
    float fract = val - (int)val;
    fract *= 10000;
    os_sprintf(target, "%d.%04d", (int)val, (int)fract);
    return os_strlen(target);
}

void ICACHE_FLASH_ATTR
http_post_cb(char * response, int http_status, char * full_response) {
    if (http_status != 200) {
        os_printf("\nPOST error, status=%d\n", http_status);
        os_printf("%s", full_response);
        os_printf("\n\n");
    }
}

static void ICACHE_FLASH_ATTR
post_readings() {
    char data[0xff];
    os_memset(data, 0, sizeof(data));
    char * head = data;

    os_strcat(head, "field1=");
    head = data + os_strlen(data);
    print_float(head, htu_temp_accum);

    os_strcat(head, "&field2=");
    head = data + os_strlen(data);
    print_float(head, htu_rh_accum);

    os_strcat(head, "&field3=");
    head = data + os_strlen(data);
    print_float(head, dht_temp_accum);

    os_strcat(head, "&field4=");
    head = data + os_strlen(data);
    print_float(head, dht_rh_accum);

    os_strcat(head, "&field5=");
    head = data + os_strlen(data);
    print_float(head, ds_temp_accum);

    os_strcat(head, "&field6=0");

    os_printf("POST: %s\n", data);
    http_post(temp_feed_url, data, http_post_cb);
}

static void ICACHE_FLASH_ATTR
post_error(uint8_t errors) {
    char data[64];
    os_memset(data, 0, sizeof(data));
    os_sprintf(data, "field6=%d", errors);
    http_post(temp_feed_url, data, http_callback_example);
}

static os_timer_t tick_timer;

static void ICACHE_FLASH_ATTR
tick(void* arg) {

    float htu_temp, htu_rh, dht_temp, dht_rh, ds_temp;
    uint8_t errors = 0;

    if (!htu21d_read_temp(&htu_temp)) {
        errors |= BIT0;
    }
    if (!htu21d_read_rh(&htu_rh)) {
        errors |= BIT1;
    }
    if (DS18B20_read(&ds_temp) < 0) {
        errors |= BIT2;
    }
    if (DHTread(&dht_temp, &dht_rh) < 0) {
        errors |= BIT3;
    }

    if (errors) {
        post_error(errors);
    } else {
        htu_temp_accum += htu_temp;
        htu_rh_accum += htu_rh;

        dht_temp_accum += dht_temp;
        dht_rh_accum += dht_rh;

        ds_temp_accum += ds_temp;

        accum_count++;
    }

    if (accum_count == SAMPLE_COUNT) {

        htu_temp_accum /= accum_count;
        htu_rh_accum /= accum_count;
        dht_temp_accum /= accum_count;
        dht_rh_accum /= accum_count;
        ds_temp_accum /= accum_count;

        post_readings();

        htu_temp_accum = htu_rh_accum = 0;
        dht_temp_accum = dht_rh_accum = 0;
        ds_temp_accum = accum_count = 0;
    }
}

static void ICACHE_FLASH_ATTR
say_hello()
{
    os_printf("Hello, sensors!\r\n");

    wifi_set_opmode( STATION_MODE );
    os_memcpy(&stationConf.ssid, ssid, 32);
    os_memcpy(&stationConf.password, password, 32);
    wifi_station_set_config(&stationConf);
    wifi_station_connect();

    htu21d_init();
    DHTInit();
    DS18B20_init();

    os_timer_disarm(&tick_timer);
    os_timer_setfn(&tick_timer, (os_timer_func_t *) tick, 0);
    os_timer_arm(&tick_timer, TICK_PAUSE, 1);
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    // Initialize UART0
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    system_set_os_print(0);
    system_init_done_cb(say_hello);
    i2c_init();
}
