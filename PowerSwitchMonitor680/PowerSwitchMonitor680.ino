#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

//needed for wifi manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <timer.h>
#include <ESP8266mDNS.h>

// https://lastminuteengineers.com/bme280-arduino-tutorial/
// BME680 https://github.com/G6EJD/BME680-Example
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include <EEPROM.h>

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
bool isSwitchPower = true;
auto timer = timer_create_default(); // create a timer with default settings
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_BME680 bme; // I2C

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int getgasreference_count = 0;

void setup()
{
  Serial.begin(115200);
  setupBme680();
  setupNetwork();
  delay(1000);
  IPAddress mqttServer = IPAddress(192, 168, 4, 100);
  client.setServer(mqttServer, 1883);
  mqttTryToConnect();
  Serial.print("Sending to ");
  Serial.print(mqttServer);
  client.publish("registered", String(ESP.getChipId()).c_str());
  //discoverMqttServers();

  // Start main processes
  timer.every(500, triggerOnPowerState);
  timer.every(30 * 1000, [](void *) -> bool {
    publishState();
    return true;
  });
}

void loop()
{
  timer.tick();
  client.loop();
}

void mqttTryToConnect()
{
  while (!client.connected())
  {
    client.connect(String(ESP.getChipId()).c_str());
    Serial.print(".");
    delay(50);
  }
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
    publishState();
  }
  if (voltage < 1 & isSwitchPower == true)
  {
    isSwitchPower = false;
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
  Serial.print("Wifi Status:");
  Serial.println(WiFi.status());
}

// Find all MQTT servers broadcasting then update for later usage.
/*void discoverMqttServers()
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
}*/

// Publish device stats in JSON format to MQTT servers.
void publishState()
{
  GetGasReference();

  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(9);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  doc["id"] = ESP.getChipId();
  doc["switch"] = isSwitchPower;
  doc["temp"] = (int)bme.readTemperature();
  doc["pressure"] = (int)(bme.readPressure() / 100.0F);
  doc["humidity"] = (int)bme.readHumidity();
  doc["gas"] = (int)gas_reference;
  doc["aiq"] = (int)AIQ();

  // Convert JSON object to string
  char buffer[119];
  size_t n = serializeJson(doc, buffer);
  mqttTryToConnect();
  boolean published = client.publish("iot", buffer, n);
  if (!published)
  {
    Serial.println("Failed to send messsage likely due to size.");
    Serial.print("Size of msg: ");
    Serial.println(n);
    Serial.println(buffer);
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
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_2X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_2X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C for 150 ms
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
  Serial.println();
  Serial.print("Gas = ");
  Serial.print(bme.readGas() / 1000.0);
  Serial.println(" KOhms");
  float air_quality_score = AIQ();
  Serial.println("Air Quality = " + String(air_quality_score, 1) + "% derived from 25% of Humidity reading and 75% of Gas reading - 100% is good quality air");
  Serial.println("Humidity element was : " + String(hum_score / 100) + " of 0.25");
  Serial.println("     Gas element was : " + String(gas_score / 100) + " of 0.75");
  if (bme.readGas() < 120000)
  {
    Serial.println("***** Poor air quality *****");
  }
  Serial.println();
  if ((getgasreference_count++) % 10 == 0)
  {
    GetGasReference();
  }
  Serial.println(CalculateIAQ(air_quality_score));
  Serial.println("------------------------------------------------");
}

void GetGasReference()
{

  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  int readings = 10;
  for (int i = 1; i <= readings; i++)
  { // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}

float AIQ()
{

  //Calculate gas contribution to IAQ index
  float gas_lower_limit = 5000;  // Bad air quality limit
  float gas_upper_limit = 50000; // Good air quality limit
  if (gas_reference > gas_upper_limit)
  {
    gas_reference = gas_upper_limit;
  }
  if (gas_reference < gas_lower_limit)
  {
    gas_reference = gas_lower_limit;
  }
  gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100;

  //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
  float air_quality_score = hum_score + gas_score;
  return air_quality_score;
}

String CalculateIAQ(float score)
{
  String IAQ_text = "Air quality is ";
  score = (100 - score) * 5;
  if (score >= 301)
    IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300)
    IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200)
    IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175)
    IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >= 51 && score <= 150)
    IAQ_text += "Moderate";
  else if (score >= 00 && score <= 50)
    IAQ_text += "Good";
  return IAQ_text;
}