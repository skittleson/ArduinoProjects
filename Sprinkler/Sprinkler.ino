#include <Adafruit_GFX.h>
#include <Adafruit_SPITFT_Macros.h>
#include <Adafruit_SPITFT.h>
#include <gfxfont.h>

#include <Adafruit_SSD1306.h>
#include <splash.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/*
  Sprinkler.cpp - App to manage a Sprinkler relay and environment data.
  Created by Spencer J Kittleson, April 27, 2020.
  Released into the public domain.
*/

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

#include <AutoConnectLocal.h>
#include <ArduinoJson.h>
#include <timer.h>
#include <ArduinoJson.h>

#define RELAY_PIN 14 // D5	IO, SCK	GPIO14
AutoConnectLocal acl;
unsigned long relayDuration = 0;
unsigned long relayDurationLast = 0;
bool relayState = false;
auto timer = timer_create_default();
char *screenMessage = "version 1.2.5";

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
float temp = 0;
float humidity = 0;
float pressure = 0;
void setup()
{
  Serial.begin(115200);
  Serial.println(screenMessage);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  displayText("Init WiFi & MQTT");
  acl.setup(String(ESP.getChipId()).c_str());
  acl.setCallback(callback);
  displayText("BME280 Sensor");
  updateBmeData();
  timer.every(1 * 1000, dashboard);
  timer.every(60 * 1000, [](void *) -> bool {
    updateBmeData();
    return true;
  });
  timer.every(60 * 1000, [](void *) -> bool {
    publishState();
    return true;
  });
}

void loop()
{
  acl.loop();
  relayLoop();
  timer.tick();
}

/*
  Turn relay on and off based on requested duration and state flag.
*/
void relayLoop()
{
  auto relayDurationLastDiff = millis() - relayDurationLast;

  // If relay is currently on and time on (diff of now minus triggered time) is greater than duration requested, turn it off.
  if (relayState && relayDurationLastDiff > relayDuration)
  {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    publishState();
  }
  if (!relayState && relayDuration > relayDurationLastDiff)
  {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    publishState();
  }
}

void callback(char *msg)
{
  StaticJsonDocument<200> doc;
  deserializeJson(doc, msg);
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
  }
  if (doc.containsKey("text"))
  {
    screenMessage = strdup(doc["text"]);
  }
}

void publishState()
{

  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(7);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  doc["id"] = String(ESP.getChipId());
  doc["relay"] = relayState;
  doc["temp"] = (int)temp;
  doc["pressure"] = (int)pressure;
  doc["humidity"] = (int)humidity;

  // Convert JSON object to string
  char jsonOutput[120];
  serializeJson(doc, jsonOutput);
  acl.publish(jsonOutput);
}

void displayText(String text)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println(text);
  display.display();
  delay(1000);
}

bool dashboard(void *)
{
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  String uptimeText = "Up: ";
  auto uptime = millis() / 1000;
  if (uptime > (60 * 20))
  {
    uptime = uptime / 60;
  }
  String fullUptime = uptimeText + uptime;
  display.print(fullUptime);
  display.setCursor(SCREEN_WIDTH - 10, 0);
  char *status = acl.status();
  if (status[0] == '\0')
  {
    display.print("|");
    display.println(screenMessage);
  }
  else
  {
    display.print("X");
    display.println(status);
  }
  display.setCursor(0, 16);
  display.print("Relay:   ");
  display.print(relayState);
  display.print("/");
  display.print(relayDuration / 1000);
  display.println(" secs");
  display.print("Humidity:");
  display.println(humidity);
  display.print("Temp:    ");
  display.println(temp);
  display.print("Pressure:");
  display.println(pressure);
  display.print("IP:      ");
  display.println(acl.ipAddress);
  display.display();
  return true;
}

void setupBme280()
{
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
}

void updateBmeData()
{
  setupBme280();
  temp = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
}
