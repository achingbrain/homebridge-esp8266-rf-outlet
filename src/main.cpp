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

#include "Arduino.h"
#include <RCSwitch.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266SSDP.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJSON.h>

extern "C" {
  #include "user_interface.h"
}

#define PORT 80
#define TX_PIN D8
#define RF_ON "110110111110011100001111"
#define RF_OFF "110110111110011100001110"
#define VERSION 1.0
#define HOST_NAME "rf-switch"

RCSwitch RF = RCSwitch();
ESP8266WebServer HTTP(PORT);
ESP8266WiFiMulti NETWORK = ESP8266WiFiMulti();
WiFiClient client;
int outletStatus = 0;
const String ON_RESPONSE = "{\"status\": \"ON\"}";
const String OFF_RESPONSE = "{\"status\": \"OFF\"}";

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
  HTTP.on("/", HTTP_GET, [](){
    Serial.println("GET / 302");
    HTTP.sendHeader("Location", "/index.html");
    HTTP.send(302, "text/plain");
  });
  HTTP.on("/index.html", HTTP_GET, [](){
    Serial.println("GET /index.html 200");
    HTTP.send(200, "text/plain", outletStatus == 1 ? "Light is on" : "Light is off");
  });
  HTTP.on("/identify", HTTP_POST, [](){
    Serial.println("POST /identify 200");
    HTTP.send(200, "application/json", outletStatus == 1 ? ON_RESPONSE : OFF_RESPONSE);

    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);

    if (outletStatus == 1) {
      delay(500);
      digitalWrite(BUILTIN_LED, LOW);
    }
  });
  HTTP.on("/outlet", HTTP_GET, [](){
    Serial.println("GET /outlet 200");
    HTTP.send(200, "application/json", outletStatus == 1 ? ON_RESPONSE : OFF_RESPONSE);
  });
  HTTP.on("/outlet", HTTP_PATCH, [](){
    StaticJsonBuffer<50> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(HTTP.arg("plain"));
    String status = root["status"];

    if (status.equalsIgnoreCase("ON")) {
      Serial.println("PATCH /outlet 200");
      RF.send(RF_ON);
      digitalWrite(BUILTIN_LED, LOW);
      HTTP.send(200, "application/json", ON_RESPONSE);
      outletStatus = 1;
    } else if (status.equalsIgnoreCase("OFF")) {
      Serial.println("PATCH /outlet 200");
      RF.send(RF_OFF);
      digitalWrite(BUILTIN_LED, HIGH);
      HTTP.send(200, "application/json", OFF_RESPONSE);
      outletStatus = 0;
    } else {
      Serial.println("PATCH /outlet 400");
      HTTP.send(400, "text/plain", "");
    }
  });
  HTTP.on("/description.xml", HTTP_GET, [](){
    HTTP.sendHeader("Content-Type", "text/xml");
    SSDP.schema(HTTP.client());
  });
  HTTP.on("/update", HTTP_POST, [](){
    Serial.println("Starting update");

    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& request = jsonBuffer.parseObject(HTTP.arg("plain"));

    if (!request.success()) {
      Serial.println("Parsing JSON failed");
      Serial.println("Could not parse " + HTTP.arg("plain"));
      HTTP.send(400, "application/json", "{}");
      return;
    }

    HTTP.send(202, "application/json", "{}");

    String remote = request["host"];
    int port = request["port"];

    Serial.print("Connecting to update server at ");
    Serial.print(remote);
    Serial.print(":");
    Serial.println(port);

    String currentVersion = "";
    String fingerprint = "";

    t_httpUpdate_return ret = ESPhttpUpdate.update(remote, port, "/update", String(VERSION), false, fingerprint, false);

    JsonObject& result = jsonBuffer.createObject();

    switch(ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        result["result"] = "error";
        result["code"] = ESPhttpUpdate.getLastError();
        result["message"] = ESPhttpUpdate.getLastErrorString();
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        result["result"] = "no-updates";
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        result["result"] = "success";
        break;
    }

    if (client.connect(remote.c_str(), port)) {
      String response;

      result.printTo(response);

      Serial.println("Sending update result");
      Serial.println(response);
      client.println("POST /result HTTP/1.0");
      client.println("Content-Type: application/json");
      client.print("Content-Length: ");
      client.println(response.length());
      client.println();
      client.print(response);
    } else {
      Serial.println("Could not send update result");
    }

    Serial.println("Rebooting");
    ESP.restart();
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

  wifi_station_set_hostname(HOST_NAME);

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

  Serial.println("Done");
}

void loop() {
  HTTP.handleClient();
}
