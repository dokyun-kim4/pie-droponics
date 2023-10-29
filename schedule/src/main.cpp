#include <WiFi.h>
#include "time.h"
#include <Arduino.h>

#define ledPin 45

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

  pinMode(ledPin, OUTPUT);
}

void lightSchedule(int startHr, int startMin, int durationHr, int durationMin)
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  now = mktime(&timeinfo);
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  // START TIME
  struct tm startTime = timeinfo;
  startTime.tm_hour = startHr;
  startTime.tm_min = startMin;
  time_t startTimestamp = mktime(&startTime);
  String startDisp = "Start at: " + String(startTime.tm_hour) + ":" + String(startTime.tm_min) + ":" + String(startTime.tm_sec);
  Serial.println(startDisp);

  // END TIME
  struct tm endTime = startTime;
  endTime.tm_hour += durationHr;
  endTime.tm_min += durationMin;
  time_t endTimestamp = mktime(&endTime);
  String endDisp = "End at: " + String(endTime.tm_hour) + ":" + String(endTime.tm_min) + ":" + String(endTime.tm_sec);
  Serial.println(endDisp);

  bool lightOn = (now >= startTimestamp && now < endTimestamp);

  if (lightOn)
  {
    Serial.println("ON");
    neopixelWrite(ledPin, 100, 150, 30);
  }
  else
  {
    Serial.println("OFF");
    neopixelWrite(ledPin, 0, 0, 0);
  }
}

void loop()
{
  delay(1000);
  lightSchedule(19, 04, 0, 1);
}