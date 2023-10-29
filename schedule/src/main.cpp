#include <WiFi.h>
#include "time.h"
#include <Arduino.h>

const char *ssid = "OLIN-DEVICES";
const char *password = "Engineering4Every1!";

const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

const char *ntpServer = "pool.ntp.org";

void setup()
{
  Serial.begin(115200);

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void loop()
{
  delay(1000);
  printLocalTime();
}