#ifndef AutoConnectLocal_h
#define AutoConnectLocal_h

#include "Arduino.h"
#include <PubSubClient.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

class AutoConnectLocal
{
public:
  AutoConnectLocal();
  void setup(const char *deviceId);
  void loop();
  boolean publish(const char *payload);
  PubSubClient client;

  /*
    On MQTT callback, trigger the builtin LED to confirm message was received
    */
  bool enableLedIndicator;
  void setCallback(void (*callback)(char *));

private:
  void mqttTryToConnect();
  void mqttCallbackFunc(char *topic, byte *payload, unsigned int length);
  IPAddress mqttDiscovery();
  void (*callback)(char *);
  const char *_deviceId;
  void connectWifi();
};

#endif
