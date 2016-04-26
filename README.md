# HomeKit compliant ESP8266 RF Outlet

Requires a NodeMCU or other ESP8266 based board, an XD-FST RF transmitter and a set of 433MHz RF remote power outlets.

Wiring:

1. ESP8266 pin 8 to RF signal
2. ESP8266 3.3v to RF Vin
3. ESP8266 Gnd to RF Gnd

The XD-FST requires 5v but seems to work with 3.3v albeit with reduced range.

## N.B.

As of the time of writing the ESP8266SSDP library has a bug in it.  Once platform.io has installed the libraries you will need to manually update `.pioenvs/nodemcuv2/ESP8266SSDP` with the [files from master](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266SSDP) and apply [esp8266/Arduino#1976](https://github.com/esp8266/Arduino/pull/1976).

## Installation

1. Update the `configureNetwork()` method in [main.cpp](./src/main.cpp) with your network credentials, then flash your ESP8266 board with the image (build with the Platform.io IDE).
2. Connect the hardware up and apply power!
3. Install homebridge-esp8266-rf-outlet:
        npm install -g homebridge-esp8266-rf-outlet
4. Edit your homebridge configuration to add the `esp8266-rf-outlet` platform:
        {
          "bridge": {
            // ... bridge settings
          },
          "platforms": [
            {
              "platform": "esp8266-rf-outlet"
            }
          ]
        }
5. Start the server
