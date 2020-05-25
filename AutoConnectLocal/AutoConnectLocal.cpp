/*
  AutoConnectLocal.cpp - Library for WiFi management with auto MQTT discovery
  Created by Spencer J Kittleson, April 21, 2020.
  Released into the public domain.
*/

#include "Arduino.h"
#include "AutoConnectLocal.h"

#include <PubSubClient.h> // https://pubsubclient.knolleary.net/api.html
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// TODO add mini web server for callback
// TODO add "roam mode" - allow a device to leave the area then re-connect. callback on reconnect to the main app. Be explicit about this feature tho.
// MAYBE ping/pong on MQTT?

/*
Auto connect to WiFi using WiFi Manager then connect to a local MQTT broker with MDNS service '_mqtt._tcp' . Some configurations are required in your local network to support this.

- On MQTT connected, a topic id  'registered' with given device id is published.
- Publish a `ping` topic id to have a `pong` with the device id published
*/
AutoConnectLocal::AutoConnectLocal()
{
  this->_deviceId = "esp";
  pinMode(LED_BUILTIN, OUTPUT);
  WiFiClient wifiClient;
  PubSubClient client(wifiClient);
  this->client = client;
}

/*
Call this on the setup on the main app with a unique device id
*/
void AutoConnectLocal::setup(const char *deviceId)
{
  this->_deviceId = deviceId;
  //WiFi.mode(WIFI_NONE_SLEEP);
  digitalWrite(LED_BUILTIN, HIGH);
  this->enableLedIndicator = true;
  Serial.print("Device Id:");
  Serial.println(this->_deviceId);
  this->connectWifi();
  this->oTA();
  this->mqttTryToConnect();
}

/*
Call this on the loop on the main app. It's non blocking.  It ensures MQTT stays connected when connected to WiFi.
*/
void AutoConnectLocal::loop()
{
  // const long resetEvery = 24 * 60 * 60 * 1000; // 24 hours * 60 mins * 60 secs * 1000 ms
  // if (millis() > resetEvery)
  // {
  //   Serial.println("Auto resetting for maintenance");
  //   delay(2000);
  //   ESP.reset();
  // }
  this->connectWifi();
  delay(1);
  ArduinoOTA.handle();
  delay(1);
  this->mqttTryToConnect();
  delay(1);
  this->client.loop();
  delay(1);
}

/*
Set callback for subscribe callback on the device id topic
*/
void AutoConnectLocal::setCallback(void (*callback)(char *))
{
  this->callback = callback;
}

/*
MQTT publish on topic 'iot' with error checking.  No device id is sent. It's recommended to send an `id` with the device id.
*/
boolean AutoConnectLocal::publish(const char *payload)
{
  boolean published = client.publish("iot", payload, sizeof(payload));
  if (!published && Serial.available())
  {
    int size = sizeof(payload);
    Serial.println("Failed to send messsage likely due to size. PubSubClient only support 120 characters. A build flag is possible with MQTT_MAX_PACKET_SIZE=256 to increase packet size.");
    Serial.println("Size of msg: ");
    Serial.print(size);
    Serial.println(payload);
  }
}

void AutoConnectLocal::mqttTryToConnect()
{
  bool connected = this->client.connected();
  if (!connected)
  {

    // use a function to set the callback in a class lib
    this->client.setCallback([this](char *topic, byte *payload, unsigned int length) { this->mqttCallbackFunc(topic, payload, length); });
    const IPAddress mqttServer = this->mqttDiscovery();
    this->client.setServer(mqttServer, 1883);
    while (!this->client.connected())
    {
      this->client.connect(this->_deviceId);
      delay(50);
    }
    this->client.subscribe(this->_deviceId);
    this->client.publish("registered", this->_deviceId);
  }
}

void AutoConnectLocal::mqttCallbackFunc(char *topic, byte *payload, unsigned int length)
{
  // Convert payload byte array to payload char array
  char msg[length];
  strncpy(msg, (char *)payload, length);
  msg[length] = 0;

  // If callback defined, then use it
  if (this->callback)
  {
    this->callback(msg);
  }

  // Indicate a message was received
  if (this->enableLedIndicator)
  {
    digitalWrite(LED_BUILTIN, LOW);  // turn the LED on (HIGH is the voltage level)
    delay(1000);                     // wait for a second
    digitalWrite(LED_BUILTIN, HIGH); // turn it off
    delay(500);
  }
}

/* Find all MQTT servers broadcasting locally but return only the first */
IPAddress AutoConnectLocal::mqttDiscovery()
{
  MDNS.begin(String(this->_deviceId));
  delay(250);
  int servicesFound = 0;
  if (Serial.available())
  {
    Serial.println("Querying for mqtt servers.");
  }
  while (servicesFound < 1)
  {
    servicesFound = MDNS.queryService("_mqtt", "_tcp"); // Send out query for esp tcp services
    delay(500);
  }
  if (Serial.available())
  {
    Serial.print("mDNS query done. Found service(s): ");
    Serial.println(servicesFound);
    Serial.println("Host: " + String(MDNS.hostname(0)));
    Serial.print("IP  : ");
    Serial.println(MDNS.IP(0));
  }

  // Support is only for the first service found
  IPAddress mqttServer = MDNS.IP(0);
  MDNS.end();
  delay(100);
  return mqttServer;
}

void AutoConnectLocal::connectWifi()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFiManager wiFiManager;
    if (!wiFiManager.autoConnect())
    {
      if (Serial.available())
      {
        Serial.println("Failed to connect to WiFi and hit timeout of WiFi portal");
      }
      delay(5000);
      ESP.reset();
    }
  }
}

void AutoConnectLocal::oTA()
{
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)
      Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

char *AutoConnectLocal::status()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return "No WiFi";
  }
  if (!this->client.connected())
  {
    return "No MQTT SRV";
  }
  return "\0";
}
