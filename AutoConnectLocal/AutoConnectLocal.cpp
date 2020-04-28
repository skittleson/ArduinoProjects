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

// TODO add "roam mode" - allow a device to leave the area then re-connect. callback on reconnect to the main app.
// TODO mqtt discovery mode

/* 
Auto connect to WiFi using WiFi Manager then connect to a local MQTT broker with MDNS service '_mqtt._tcp' . Some configurations are required in your local network to support this. 

- On MQTT connected, a topic id  'registered' with given device id is published.
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
    digitalWrite(LED_BUILTIN, LOW);
    this->enableLedIndicator = true;
    Serial.print("Device Id:");
    Serial.println(deviceId);
    WiFiManager wiFiManager;
    if (wiFiManager.autoConnect())
    {
        wiFiManager.autoConnect(); // Wait for a WiFi network to either by found or setup by a user.
        this->mqttDiscovery();
        IPAddress mqttServer = IPAddress(192, 168, 4, 100);
        this->client.setServer(mqttServer, 1883);
        this->client.setCallback([this](char *topic, byte *payload, unsigned int length) { this->mqttCallbackFunc(topic, payload, length); }); // use a function to set the callback in a class lib
        this->mqttTryToConnect();
        Serial.println("Ready");
    }
    else
    {
        Serial.println("Failed to connect to WiFi and hit timeout of WiFi portal");
        ESP.reset();
        delay(1000);
    }
}

/* 
Call this on the loop on the main app. It's non blocking.  It ensures MQTT stays connected when connected to WiFi. 
*/
void AutoConnectLocal::loop()
{
    this->mqttTryToConnect();
    this->client.loop();
}

/*
Set callback for subscribe callback on the device id topic
*/
void AutoConnectLocal::setCallback(void (*callback)(char *))
{
    this->callback = callback;
}

/*
MQTT publish on topic 'iot' with error checking.
*/
boolean AutoConnectLocal::publish(const char *payload)
{
    this->mqttTryToConnect();
    boolean published = client.publish("iot", payload, sizeof(payload));
    if (!published)
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
        Serial.print("Attempting to connect to MQTT server");
        while (!this->client.connected())
        {
            this->client.connect(this->_deviceId);
            Serial.print(".");
            delay(50);
        }
        Serial.println("");
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
        callback(msg);
    }

    // Indicate a message was received
    if (this->enableLedIndicator)
    {
        digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
        delay(1500);                     // wait for a second
        digitalWrite(LED_BUILTIN, LOW);  // turn it off
        delay(1000);
    }
}

/* Find all MQTT servers broadcasting locally */
void AutoConnectLocal::mqttDiscovery()
{
    MDNS.begin(String(this->_deviceId));
    delay(250);
    int servicesFound = 0;
    Serial.println("Querying for mqtt servers.");
    while (servicesFound < 1)
    {
        servicesFound = MDNS.queryService("_mqtt", "_tcp"); // Send out query for esp tcp services
        Serial.print(".");
        delay(500);
    }
    Serial.print("mDNS query done. Found service(s): ");
    Serial.println(servicesFound);
    for (int i = 0; i < servicesFound; ++i)
    {

        // Print details for each service found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(MDNS.hostname(i));
        Serial.print(" (");
        Serial.print(MDNS.IP(i));
        Serial.print(":");
        Serial.print(MDNS.port(i));
        Serial.println(")");
        // if (i < MQTT_SERVER_LIMIT)
        // {
        //     mqttServers[i] = MDNS.IP(i);
        // }
    }

    // Support is only for the first service found

    MDNS.end();
}
