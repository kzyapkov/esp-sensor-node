ESP8266 sensor node demo
------------------------

This reads, averages and posts readings from 3 sensors:
  * HTU21D
  * DHT22 (packed as AM2303)
  * DS18B20

Readings are posted to a thingspeak channel for later analysis or whatever.
My sensor node logs [here](https://thingspeak.com/channels/33253).

Code is stolen from various places, disclaimer and license notes retained where present.

To deploy your own, copy `user/config.sample.h` to `user/config.h`, edit,
build and upload. Connections, suitable for an ESP-01 module:

    I2C DATA ------------------ GPIO0
    I2C CLK ------------------- GPIO2
    DHT22 data pin ------------ GPIO1 (U0TXD)
    DS18B20 data pin ---------- GPIO3 (U0RXD)
