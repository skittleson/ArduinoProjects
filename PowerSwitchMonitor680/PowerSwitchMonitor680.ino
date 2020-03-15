#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// needed for wifi manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <timer.h>
#include <ESP8266mDNS.h>

// https://lastminuteengineers.com/bme280-arduino-tutorial/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/*
  - Trigger a mqtt message on power state changes
  - Every minute send an update of state

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

const char *mdnsService = "_mqtt";
#define MQTT_SERVER_LIMIT 10 // store a max of 10 mqtt servers
#define SEALEVELPRESSURE_HPA (1013.25)
IPAddress mqttServers[MQTT_SERVER_LIMIT] = {};
bool isSwitchPower = true;
auto timer = timer_create_default(); // create a timer with default settings
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_BME680 bme; // I2C

void setup()
{
  Serial.begin(115200);
  setupBme680();
  setupNetwork();
  delay(1000);
  discoverMqttServers();

  // Start main processes
  timer.every(100, triggerOnPowerState);
  timer.every(60 * 1000, [](void *) -> bool {
    publishState();
    printBme();
    return true;
  });
}

void loop()
{
  timer.tick();
}

bool triggerOnPowerState(void *)
{
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);

  // Toggle state when power is turned on / off
  if (voltage > 3 & isSwitchPower == false)
  {
    isSwitchPower = true;
    Serial.println(voltage);
    publishState();
  }
  if (voltage < 1 & isSwitchPower == true)
  {
    isSwitchPower = false;
    Serial.println(voltage);
    publishState();
  }
  return true;
}

void setupNetwork()
{
  delay(10);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  delay(10);
  Serial.println(WiFi.localIP());
}

// Find all MQTT servers broadcasting then update for later usage.
void discoverMqttServers()
{

  MDNS.begin(String(ESP.getChipId()).c_str());
  delay(250);
  int n = MDNS.queryService(mdnsService, "_tcp"); // Send out query for esp tcp services
  Serial.println("mDNS query done");

  // Empty IPAddress spaces
  for (int i = 0; i < MQTT_SERVER_LIMIT; i++)
  {
    mqttServers[i] = IPAddress();
  }
  if (n == 0)
  {
    Serial.println("no services found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" service(s) found");
    for (int i = 0; i < n; ++i)
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
      if (i < MQTT_SERVER_LIMIT)
      {
        mqttServers[i] = MDNS.IP(i);
      }
    }
  }
  Serial.println();
  MDNS.end();
}

// Publish device stats in JSON format to MQTT servers.
void publishState()
{
  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  int uptime = millis();
  doc["id"] = String(ESP.getChipId());
  doc["switchPower"] = isSwitchPower;
  doc["uptime"] = uptime;
  doc["temp"] = bme.readTemperature();
  doc["pressure"] = bme.readPressure() / 100.0F;
  doc["humidity"] = bme.readHumidity();
  doc["gas"] = bme.readGas() / 1000.0;

  // Convert JSON object to string
  char jsonOutput[512];
  serializeJson(doc, jsonOutput);

  // Send to all MQTT servers
  for (int i = 0; i < MQTT_SERVER_LIMIT; i++)
  {
    const char *testIpAddress = mqttServers[i].toString().c_str();
    if (IPAddress().fromString(testIpAddress))
    {
      Serial.println(mqttServers[i]);

      // Publish to all MQTT servers on port 1883 with chip id as identifer
      client.setServer(mqttServers[i], 1883);
      if (client.connect(String(ESP.getChipId()).c_str()))
      {
        // The topic is `iot`. If many devices are present, use the `id` as a way to filter specfic devices.
        client.publish("iot", jsonOutput);
      }
    }
  }
}

void setupBme680()
{
  if (!bme.begin(0x77))
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
      ;
  }
  printBme();
}

void printBme()
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println("*C");
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println("hPa");
  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println("m");
  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println("%");
  Serial.print("Gas = ");
  Serial.print(bme.readGas() / 1000.0);
  Serial.println(" KOhms");
}
