#include <AutoConnectLocal.h>
#include <ArduinoJson.h>
#include <timer.h>

// https://lastminuteengineers.com/bme280-arduino-tutorial/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

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
#define SEALEVELPRESSURE_HPA (1013.25)
bool isSwitchPower = true;
auto timer = timer_create_default(); // create a timer with default settings
Adafruit_BME280 bme;                 // I2C
AutoConnectLocal acl;

void setup()
{
  Serial.begin(115200);
  setupBme280();
  acl.enableLedIndicator = true;
  acl.setup(String(ESP.getChipId()).c_str());
  timer.every(250, triggerOnPowerState);
  timer.every(60 * 1000, [](void *) -> bool {
    publishState();
    return true;
  });
}

void loop()
{
  timer.tick();
  acl.loop();
}

bool triggerOnPowerState(void *)
{
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);

  // Toggle state when power is turned on / off
  if (voltage > 4.25 & isSwitchPower == false)
  {
    isSwitchPower = true;
    publishState();
  }
  if (voltage < 4 & isSwitchPower == true)
  {
    isSwitchPower = false;
    publishState();
  }
  return true;
}

// Publish device stats in JSON format to MQTT servers.
void publishState()
{

  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(6);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  doc["id"] = String(ESP.getChipId());
  doc["switch"] = isSwitchPower;
  doc["temp"] = (int)bme.readTemperature();
  doc["pressure"] = (int)(bme.readPressure() / 100.0F);
  doc["humidity"] = (int)bme.readHumidity();

  // Convert JSON object to string
  char jsonOutput[120];
  serializeJson(doc, jsonOutput);
  acl.publish(jsonOutput);

  // // Send to all MQTT servers
  // for (int i = 0; i < MQTT_SERVER_LIMIT; i++) {
  //   const char *testIpAddress = mqttServers[i].toString().c_str();
  //   if (IPAddress().fromString(testIpAddress)) {
  //     Serial.println(mqttServers[i]);

  //     // Publish to all MQTT servers on port 1883 with chip id as identifer
  //     client.setServer(mqttServers[i], 1883);
  //     if (client.connect(String(ESP.getChipId()).c_str()))
  //     {
  //       // The topic is `iot`. If many devices are present, use the `id` as a way to filter specfic devices.
  //       client.publish("iot", jsonOutput);
  //     }
  //   }
  // }
}

void setupBme280()
{
  if (!bme.begin(0x76))
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
  printBme280();
}

void printBme280()
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
  Serial.println();
}
