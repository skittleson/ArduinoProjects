#include "Arduino.h"
#include "AutoConnectLocal.h"

#include <PubSubClient.h> // https://pubsubclient.knolleary.net/api.html
#include <ESP8266WiFi.h>  //https://github.com/esp8266/Arduino
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <WiFiUdp.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);
bool mqttClientConnected = false;
bool mqttTriggered = false;
char *mqttMsg;
String ipAddress;

/*
  Resources: https://randomnerdtutorials.com/guide-for-oled-display-with-arduino/

  Pin Layout
  https://wiki.wemos.cc/products:d1:d1_mini_lite
  Pin	Function	ESP-8266 Pin
  TX	TXD	TXD
  RX	RXD	RXD
  A0	Analog input, max 3.3V input	A0
  D0	IO	GPIO16
  D1	IO, SCL	GPIO5
  D2	IO, SDA	GPIO4
  D3	IO, 10k Pull-up	GPIO0
  D4	IO, 10k Pull-up, BUILTIN_LED	GPIO2
  D5	IO, SCK	GPIO14
  D6	IO, MISO	GPIO12
  D7	IO, MOSI	GPIO13
  D8	IO, 10k Pull-down, SS	GPIO15
  G	Ground	GND
  5V	5V	-
  3V3	3.3V	3.3V
  RST	Reset	RST
*/

void connectWifi(const char *deviceId)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Device Id: ");
    Serial.println(deviceId);
    WiFiManager wiFiManager;
    if (!wiFiManager.autoConnect(deviceId))
    {
      delay(5000);
      ESP.reset();
    }
    ipAddress = WiFi.localIP().toString();
  }
}

char *getStatus()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    return "No WiFi\0";
  }
  if (mqttClientConnected)
  {
    return "No MQTT SRV\0";
  }
  return "\0";
}

void mqttTryToConnect(const char *deviceId)
{
  mqttClientConnected = false;
  bool connected = client.connected();
  if (!connected)
  {

    // use a function to set the callback in a class lib
    //const IPAddress mqttServer = mqttDiscovery();
    //const char *ip = "192.168.4.100";
    //client.setServer(ip, 1883);
    setMqttClientFromDiscovery();
    client.setCallback(mqttCallbackFunc);
    while (!client.connected())
    {
      client.connect(deviceId);
      Serial.print(".");
      delay(50);
    }
    mqttClientConnected = true;
    client.subscribe(deviceId);
    client.publish("registered", deviceId);
  }
  else
  {
    mqttClientConnected = true;
  }
}

/* Find all MQTT servers broadcasting locally but return only the first */
void setMqttClientFromDiscovery()
{
  MDNS.begin(String(deviceId));
  delay(250);
  int servicesFound = 0;
  while (servicesFound < 1)
  {
    servicesFound = MDNS.queryService("_mqtt", "_tcp"); // Send out query for esp tcp services
    delay(50);
  }

  // Support is only for the first service found
  client.setServer(MDNS.IP(0), 1883);
  delay(1);
  MDNS.end();
  delay(1);
}

void mqttCallbackFunc(char *topic, byte *payload, unsigned int length)
{

  // Convert payload byte array to payload char array
  char msg[length];
  strncpy(msg, (char *)payload, length);
  msg[length] = 0;
  mqttMsg = msg;
  mqttTriggered = true;
}

boolean publish(const char *payload)
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

void connectivityLoop(const char *deviceId)
{
  connectWifi(deviceId);
  mqttTryToConnect(deviceId);
  client.loop();
  mqttPopMessage();
  delay(1);
}

void mqttPopMessage()
{
  if (mqttTriggered)
  {
    mqttTriggered = false;

    // Callback is defined in the primary INO
    if (callback)
    {
      callback(mqttMsg);
    }
  }
}

String lastIpAddress()
{
  return ipAddress;
}
