#include <Arduino.h>
#include <WiFi.h>
#include "set_keys.h"


String ssid = "OLIN-DEVICES";

void setup()
{
  Serial.begin(115200);
  // Connect to Wi-Fi
  WiFi.begin(ssid, getWifiKey());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop()
{
  // Your code here
}

