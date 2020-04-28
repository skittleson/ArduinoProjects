/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <ESP8266WiFi.h> //https://github.com/esp8266/Arduino
//#include <EEPROM.h>
#include "FS.h"
#include <ArduinoJson.h>
const char *store = "/wifi.json";

void setup()
{
  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");
  // File f = SPIFFS.open("/file.txt", "w");
  // f.println("This is the first line ");
  // f.println("This is the second line ");
  // f.close();
  // delay(1000);
}

void loop()
{

  /*
FSInfo fs_info;
SPIFFS.info(fs_info);
*/

  SPIFFS.begin();
  StaticJsonDocument<1024> doc;
  if (SPIFFS.exists(store))
  {
    File file = SPIFFS.open(store, "r");
    deserializeJson(doc, file);
    Serial.println("From memory");
    serializeJson(doc, Serial);
    Serial.println("");
    file.close();
  }
  doc["1"] = 1;

  // write to file
  File fileWrite = SPIFFS.open(store, "w");
  serializeJson(doc, fileWrite);
  fileWrite.close();
  SPIFFS.end();

  //doc["2"] = 2;
  //doc["1"]["test"] = 1;
  // Serial.println("Now: ");
  // serializeJson(doc, Serial);
  //delay(1000);

  // File fileWrite = SPIFFS.open(store, "w");
  // serializeJson(doc, fileWrite);
  // fileWrite.close();

  // File f = SPIFFS.open("/file.txt", "r");
  // while (f.available())
  // {
  //   String line = f.readStringUntil('\n');
  //   Serial.println(line);
  // }
  // f.close();
  // delay(10 * 1000);
  Serial.println("scan start");

  delay(50);
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {

      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(") ");
      Serial.print(WiFi.BSSIDstr(i));
      Serial.print(" ");
      Serial.println(WiFi.encryptionType(i));
      //Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  Serial.println("");

  // Wait a bit before scanning again
  delay(60 * 1000);
}

void StoreData()
{
}

// void writeString(int address, String data)
// {
//   EEPROM.begin(512);
//   int stringSize = data.length();
//   for (int i = 0; i < stringSize; i++)
//   {
//     EEPROM.write(address + i, data[i]);
//   }
//   EEPROM.write(address + stringSize, '\0'); //Add termination null character
//   EEPROM.end();
// }

// String readString(int address)
// {
//   const int memSize = 512;
//   EEPROM.begin(memSize);
//   char data[100]; //Max 100 Bytes
//   int len = 0;
//   unsigned char k;
//   k = EEPROM.read(address);
//   while (k != '\0' && len < memSize) //Read until null character
//   {
//     k = EEPROM.read(address + len);
//     data[len] = k;
//     len++;
//   }
//   data[len] = '\0';
//   EEPROM.end();
//   return String(data);
// }