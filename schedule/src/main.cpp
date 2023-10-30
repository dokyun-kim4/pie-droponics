#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
#include "password.h"

#define ledPin 45

// WIFI CREDENTIALS
const char *ssid = "OLIN-DEVICES";
const char *ntpServer = "pool.ntp.org";

// CONSTANTS FOR TIME OFFSET
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

void setup()
{
  Serial.begin(115200);

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // GET TIME FROM WIFI
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

  // END TIME
  struct tm endTime = startTime;
  endTime.tm_hour += durationHr;
  endTime.tm_min += durationMin;
  time_t endTimestamp = mktime(&endTime);

  // CHECK IF TIME IS WITHIN SPECIFIED DURATION
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
  lightSchedule(15, 20, 0, 1);
}