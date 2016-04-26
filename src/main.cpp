/*
1
On:  110110111110011100001111
Off: 110110111110011100001110

2
On:  110110111110011100001101
Off: 110110111110011100001100

3
On:  110110111110011100001011
Off: 110110111110011100001010

4
On:  110110111110011100000111
Off: 110110111110011100000110

All
On:  110110111110011100000010
Off: 110110111110011100000001
*/

#define DEBUG_SSDP Serial
#define DEBUG_WIFI_MULTI Serial.printf

#include "Arduino.h"

#include <RCSwitch.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266SSDP.h>

#define PORT 80
#define TX_PIN D8

RCSwitch RF = RCSwitch();
ESP8266WebServer HTTP(PORT);
ESP8266WiFiMulti NETWORK = ESP8266WiFiMulti();

void configureSwitch() {
  RF.enableTransmit(TX_PIN);
  RF.setProtocol(1);
  RF.setPulseLength(232);
  RF.setRepeatTransmit(2);
}

void configureNetwork() {
  NETWORK.addAP("SSID", "PASSWORD");

  while (NETWORK.run() != WL_CONNECTED) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
  }

  digitalWrite(BUILTIN_LED, HIGH);

  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  IPAddress myIp = WiFi.localIP();
  Serial.printf("IP: %d.%d.%d.%d\n", myIp[0], myIp[1], myIp[2], myIp[3]);
}

void configureHttp() {
  HTTP.on("/index.html", HTTP_GET, [](){
    HTTP.send(200, "text/plain", "Hello World!");
  });
  HTTP.on("/light/on", HTTP_GET, [](){
    Serial.println("Turning light on");
    RF.send("110110111110011100001111");
    HTTP.send(200, "text/plain", "Light on");
    digitalWrite(BUILTIN_LED, LOW);
  });
  HTTP.on("/light/off", HTTP_GET, [](){
    Serial.println("Turning light off");
    RF.send("110110111110011100001110");
    HTTP.send(200, "text/plain", "Light off");
    digitalWrite(BUILTIN_LED, HIGH);
  });
  HTTP.on("/description.xml", HTTP_GET, [](){
    HTTP.sendHeader("Content-Type", "text/xml");
    SSDP.schema(HTTP.client());
  });
  HTTP.begin();
}

void configureSsdp() {
  SSDP.setSchemaURL("description.xml");
  SSDP.setHTTPPort(80);
  SSDP.setName("ESP8266 RF Outlet");
  SSDP.setDeviceType("urn:schemas-upnp-org:device:ESP8266RFOutlet");
  SSDP.setSerialNumber("00000000001");
  SSDP.setURL("index.html");
  SSDP.setModelName("ESP8266 RF Outlet");
  SSDP.setModelNumber("00000000001");
  SSDP.setModelURL("https://github.com/achingbrain/homebridge-esp8266-rf-outlet");
  SSDP.setManufacturer("AchingBrain");
  SSDP.setManufacturerURL("http://achingbrain.net");
  SSDP.begin();
}

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.println("Configuring RF");
  configureSwitch();
  Serial.println("Configuring Network");
  configureNetwork();
  Serial.println("Configuring HTTP");
  configureHttp();
  Serial.println("Configuring SSDP");
  configureSsdp();

  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(100);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop() {
  HTTP.handleClient();
}
