#include <ArduinoJson.h>
#include <timer.h>

// https://lastminuteengineers.com/bme280-arduino-tutorial/
// BME680 https://github.com/G6EJD/BME680-Example
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

#define SEALEVELPRESSURE_HPA (1013.25)
bool isSwitchPower = true;
auto timer = timer_create_default(); // create a timer with default settings
AutoConnectLocal acl;
Adafruit_BME680 bme; // I2C

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int getgasreference_count = 0;

void setup()
{
  Serial.begin(115200);
  setupBme680();
  acl.setup(String(ESP.getChipId()).c_str());

  // Start main processes
  timer.every(10 * 1000, [](void *) -> bool {
    GetGasReference();
    return true;
  });
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
  //doc["aiq"] = (int)AIQ();

  // Convert JSON object to string
  char jsonOutput[120];
  serializeJson(doc, jsonOutput);
  acl.publish(jsonOutput);
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
