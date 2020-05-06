/*
  Sprinkler.cpp - App to manage a Sprinkler relay and environment data.
  Created by Spencer J Kittleson, April 27, 2020.
  Released into the public domain.
*/

/*
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

#include <AutoConnectLocal.h>
#include <ArduinoJson.h>

#define RELAY_PIN 14 // D5	IO, SCK	GPIO14
AutoConnectLocal acl;
unsigned long relayDuration = 0;
unsigned long relayDurationLast = 0;
bool relayState = false;

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  acl.setup(String(ESP.getChipId()).c_str());
  acl.setCallback(callback);
}

void loop()
{
  acl.loop();
  relayLoop();
}

void relayLoop()
{
  auto relayDurationLastDiff = millis() - relayDurationLast;

  // If relay is currently on and time on (diff of now minus triggered time) is greater than duration requested, turn it off.
  if (relayState && relayDurationLastDiff > relayDuration)
  {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Relay off");
  }
  if (!relayState && relayDuration > relayDurationLastDiff)
  {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Relay on");
  }
}

void callback(char *msg)
{
  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);
  serializeJson(doc, Serial);
  Serial.println("");
  if (doc.containsKey("duration"))
  {
    relayDuration = doc["duration"].as<unsigned long>();
    relayDurationLast = millis();

    // runaway protection for triggering on a relay longer than needed
    auto timeout = 30 * (60 * 1000);
    if (relayDuration > timeout)
    {
      relayDuration = timeout;
    }
    Serial.print("Running relay for: ");
    Serial.println(relayDuration);
  }
}
