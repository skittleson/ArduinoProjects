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
  Discover and connect to a single MQTT server in a network. 
  - Publish regular temp, humidity, and pressure stats every minute.
  - Subscribe to /iot/<chip id> for commands to trigger a relay based on duration.
*/

const char *mdnsService = "_mqtt";
#define SEALEVELPRESSURE_HPA (1013.25)
#define RELAY_PIN (2)
WiFiClient wifiClient;
PubSubClient client(wifiClient);
Adafruit_BME680 bme; // I2C
unsigned long uptime;
unsigned long lastPublishTime;
unsigned long relayRunUntilTime;

void setup()
{
  Serial.begin(115200);
  // setupBme680();
  setupNetwork();
  String discoveredMQTTServers[1];
  discoverMqttServers(discoveredMQTTServers);

  // Use the MQTT server found to listen
  client.setServer(discoveredMQTTServers[i].c_str(), 1883);
  client.setCallback(callback);
  uptime = 0;
  lastPublishTime = 1000 * 70;
  relayRunUntilTime = 0;
}

void loop()
{
  // The wifi doesn't always stick around, so reconnect if not connected.
  reconnectIfNotConnected();
  reconnectMqtt();
  uptime = millis();

  // Publish sensor stats roughly every minute.
  unsigned long timeDiff = uptime - lastPublishTime;
  if (timeDiff > 60000)
  {
    publishState(true);
    lastPublishTime = millis();
  }
  if (uptime > relayRunUntilTime)
  {
    // Turn off the relay
    digitalWrite(RELAY_PIN, HIGH);
    // TODO turn off relay if over time.
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Set run time of relay and turn on.
  relayRunUntilTime = milliS() + (10 * 1000 * 60);
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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

void reconnectMqtt()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(String(ESP.getChipId()).c_str()))
    {
      Serial.println("connected");
      subscribe
          String topic = "iot/";
      topic += String(ESP.getChipId());
      client.subscribe(topic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

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
void publishState(bool relayState)
{

  // Provision capacity for JSON doc
  const int capacity = JSON_OBJECT_SIZE(8);
  StaticJsonDocument<capacity> doc;
  JsonObject obj = doc.to<JsonObject>();

  // Collect and build stat JSON object
  doc["id"] = String(ESP.getChipId());
  doc["uptime"] = uptime;
  doc["relay"] = relayState;
  doc["temp"] = bme.readTemperature();
  doc["pressure"] = bme.readPressure() / 100.0F;
  doc["humidity"] = bme.readHumidity();

  // Convert JSON object to string
  char jsonOutput[512];
  serializeJson(doc, jsonOutput);
  client.publish("iot", jsonOutput);
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
}
