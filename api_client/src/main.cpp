#include <Arduino.h>
#include <WiFi.h>
#include "api_client.h"
#include "set_keys.h"

const char* ssid = "OLIN-DEVICES";
String base = "https://dhcqign8ai.execute-api.us-east-2.amazonaws.com/dev/";

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
  Serial.println("Enter the API route and press Enter:");
  while (Serial.available() == 0) {
    delay(100); // Wait for user input
  }
  // Read the user input and store it in the 'route' variable
  String route = Serial.readStringUntil('\n');
	String Url;
  Url = base + route;
  Url.trim();
  String response = getRequest(Url);
}