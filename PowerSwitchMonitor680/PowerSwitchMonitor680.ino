#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino

// needed for wifi manager library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>

// https://lastminuteengineers.com/bme280-arduino-tutorial/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

/*
  - Trigger a mqtt message on power state changes
  - Every minute send sensor  stats
*/

const char *mdnsService = "_mqtt";
#define DELAY_MS 100
#define MQTT_SERVER_LIMIT 10 // store a max of 10 mqtt servers
#define SEALEVELPRESSURE_HPA (1013.25)
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_BME680 bme; // I2C
String discoveredMQTTServers[MQTT_SERVER_LIMIT];
unsigned long uptime;
unsigned long lastPublishTime;

void setup()
{
  Serial.begin(115200);
  setupBme680();
  setupNetwork();
  discoverMqttServers(discoveredMQTTServers);
  uptime = 0;

  // Default this so it triggers on first loop
  lastPublishTime = 1000 * 70;
}

void loop()
{
  // The wifi doesn't always stick around, so reconnect if not connected.
  reconnectIfNotConnected();
  uptime = millis();

  // Notified all MQTT servers of power state changes
  bool power = triggerOnPowerState(discoveredMQTTServers);

  // Publish sensor stats roughly every minute.
  unsigned long timeDiff = uptime - lastPublishTime;
  if (timeDiff > 60000)
  {
    publishState(power, discoveredMQTTServers);
    lastPublishTime = millis();
  }
  delay(DELAY_MS);
}

bool triggerOnPowerState(String ipAddresses[])
{
  // Maintain previous state using static keyword for trigging a publish.
  static int isSwitchPower;
  bool power = isPowerOn();

  // Toggle state when power is turned on / off
  if (power & isSwitchPower == false)
  {
    isSwitchPower = true;
    publishState(power, ipAddresses);
  }
  if (!power & isSwitchPower == true)
  {
    isSwitchPower = false;
    publishState(power, ipAddresses);
  }
  return power;
}

bool isPowerOn()
{
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.0 / 1023.0);
  return voltage > 3.5;
}

#pragma region Networking
void setupNetwork()
{
  delay(10);
  WiFiManager wifiManager;
  wifiManager.autoConnect();
  delay(10);
  Serial.println(WiFi.localIP());
}

void reconnectIfNotConnected()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.reconnect();
    delay(500);
    Serial.print(".");
  }
}

String IpAddress2String(const IPAddress &ipAddress)
{
  return String(ipAddress[0]) + String(".") +
         String(ipAddress[1]) + String(".") +
         String(ipAddress[2]) + String(".") +
         String(ipAddress[3]);
}
#pragma endregion

// Find all MQTT servers broadcasting then ipAddress array
void discoverMqttServers(String ipAddresses[])
{
  MDNS.begin(String(ESP.getChipId()).c_str());
  delay(250);
  int n = 0;
  while (n < 1)
  {
    // Send out query for esp tcp services
    n = MDNS.queryService(mdnsService, "_tcp");
    Serial.println("mDNS query done");
    if (n == 0)
    {
      Serial.println("no services found... query until at least one is found");
      delay(100);
    }
  }

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
      ipAddresses[i] = IpAddress2String(MDNS.IP(i));
    }
  }
  Serial.println();
  MDNS.end();
}

// Publish device stats in JSON format to MQTT servers.
void publishState(bool powerState, String ipAddresses[])
{

  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  doc["id"] = String(ESP.getChipId());
  doc["switchPower"] = powerState;
  doc["uptime"] = uptime;
  doc["temp"] = bme.readTemperature();
  doc["pressure"] = bme.readPressure() / 100.0F;
  doc["humidity"] = bme.readHumidity();
  doc["gas"] = bme.readGas() / 1000.0;

  // Convert JSON object to string
  char jsonOutput[512];
  serializeJson(doc, jsonOutput);
  for (int i = 0; i < MQTT_SERVER_LIMIT; i++)
  {
    const char *testIpAddress = ipAddresses[i].c_str();
    if (IPAddress().fromString(testIpAddress))
    {
      Serial.println(ipAddresses[i]);

      // Publish to all MQTT servers on port 1883 with chip id as identifer
      client.setServer(ipAddresses[i].c_str(), 1883);
      if (client.connect(String(ESP.getChipId()).c_str()))
      {
        // The topic is `iot`. If many devices are present, use the `id` as a way to filter specfic devices.
        Serial.println("published message");
        client.publish("iot", jsonOutput);
        client.disconnect();
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
