#include <AutoConnectLocal.h>

#define LIGHT 5 // D1	IO, SCL	GPIO5
AutoConnectLocal acl;

void setup()
{
  Serial.begin(115200);
  pinMode(LIGHT, OUTPUT);
  acl.setup(String(ESP.getChipId()).c_str());
  acl.setCallback(callback);
}

void loop()
{
  acl.loop();
}

void callback(char *msg)
{
  Serial.println(msg);
  ledFadeInOut();
}

void ledFadeInOut()
{
  const int loopCount = 128;
  const int speed = 30;
  for (int i = 0; i < loopCount; i++)
  {
    analogWrite(LIGHT, loopCount - (i * 2));
    delay(speed);
  }
  for (int i = 0; i < loopCount; i++)
  {
    analogWrite(LIGHT, (i * 2));
    delay(speed);
  }
}

void ledToggle(boolean isOn)
{
  if (isOn)
  {
    analogWrite(LIGHT, 255);
  }
  else
  {
    analogWrite(LIGHT, 0);
  }
}
