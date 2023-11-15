#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "../include/secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "Adafruit_SHT4x.h"
#include <ArduinoJson.h>
#include <Wire.h>

// --------------- AWS setup ------------------------------//
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "sensor/"
#define AWS_IOT_SUBSCRIBE_TOPIC "cmd/"
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

// --------------- ESP setup -------------------------//
#define pumpPin 13
#define lightPin 14

uint32_t prevMillis;
int pumpOnDuration = 3000;  // how long to turn on
int pumpOffDuration = 1000; // how long to turn off
bool pumpIsOff = true;

const char *lightStartTime = "2002";
const char *lightEndTime = "2010";

// -------------- Time setup ----------------------- //
const char *ntpServer = "pool.ntp.org";
// CONSTANTS FOR TIME OFFSET
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

// -------------------- AWS functions ----------------//
// void setupSHT40()
// {
//   // Setup for temp/humidity sensor
//   if (!sht4.begin())
//   {
//     Serial.println("Couldn't find SHT4x sensor");
//     while (1)
//       delay(1);
//   }
//   Serial.println("Found SHT4x sensor");
//   Serial.print("Serial number 0x");
//   Serial.println(sht4.readSerial(), HEX);

//   // You can have 3 different precisions, higher precision takes longer
//   sht4.setPrecision(SHT4X_HIGH_PRECISION);

//   // You can have 6 different heater settings
//   sht4.setHeater(SHT4X_NO_HEATER);
// }

// // Create an alias for temperature and humidity readings
// typedef std::pair<float, float> TempHumidReading;

// // Function to get temperature and humidity readings
// TempHumidReading getTempHumid()
// {
//   sensors_event_t humidity, temp;
//   sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
//   return {temp.temperature, humidity.relative_humidity};
// }

// void messageHandler(String &topic, String &payload)
// {
//   StaticJsonDocument<200> doc;
//   deserializeJson(doc, payload);
//   const char *target = doc["target"];
//   int onDuration = int(doc["on_interval"]);
//   int offDuration = int(doc["off_interval"]);

//   if (strcmp(target, "pump") == 0)
//   {
//     pumpStartTime = doc["start_time"];
//     pumpOnDuration = int(onDuration);
//     pumpOffDuration = int(offDuration);
//   }
//   else if (strcmp(target, "light") == 0)
//   {
//     lightStartTime = doc["start_time"];

//     lightOnDuration = int(onDuration);
//     lightOffDuration = int(offDuration);
//   }
// }

// void connectAWS()
// {
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

//   Serial.println("Connecting to Wi-Fi");

//   while (WiFi.status() != WL_CONNECTED)
//   {
//     delay(500);
//     Serial.print(".");
//   }

//   // Configure WiFiClientSecure to use the AWS IoT device credentials
//   net.setCACert(AWS_CERT_CA);
//   net.setCertificate(AWS_CERT_CRT);
//   net.setPrivateKey(AWS_CERT_PRIVATE);

//   // Connect to the MQTT broker on the AWS endpoint we defined earlier
//   client.begin(AWS_IOT_ENDPOINT, 8883, net);

//   // Create a message handler
//   client.onMessage(messageHandler);

//   Serial.print("Connecting to AWS IOT");

//   while (!client.connect(THINGNAME))
//   {
//     Serial.print(".");
//     delay(100);
//   }

//   if (!client.connected())
//   {
//     Serial.println("AWS IoT Timeout!");
//     return;
//   }

//   // Subscribe to a topic
//   client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
//   Serial.println("AWS IoT Connected!");
// }

// void publishMessage()
// {
//   StaticJsonDocument<200> doc;
//   doc["time"] = millis();
//   TempHumidReading tempHumidReading = getTempHumid();
//   doc["temp"] = tempHumidReading.first;
//   doc["humidity"] = tempHumidReading.second;

//   char jsonBuffer[512];
//   serializeJson(doc, jsonBuffer); // print to client
//   client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
// }

// -------------------- ESP control functions ------------ //
void connectWifi()
{
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
}

void setup()
{
  Serial.begin(115200);
  pinMode(pumpPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  connectWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void runPump()
{
  int currentMillis = millis();

  if (pumpIsOff)
  {

    if (currentMillis - prevMillis >= pumpOffDuration)
    {
      pumpIsOff = false;
      prevMillis = currentMillis;
      digitalWrite(pumpPin, HIGH);
    }
  }
  else if (currentMillis - prevMillis >= pumpOnDuration)
  {
    pumpIsOff = true;
    prevMillis = currentMillis;
    digitalWrite(pumpPin, LOW);
  }
}

void runLight()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return;
  }
  now = mktime(&timeinfo);

  // START TIME
  struct tm lightStartTimeStruct = timeinfo;

  // Convert HHMM format to hour and minutes
  char str[5];
  strncpy(str, lightStartTime, 2);
  lightStartTimeStruct.tm_hour = atoi(str);
  strcpy(str, lightStartTime);
  lightStartTimeStruct.tm_min = atoi(lightStartTime + 2);
  lightStartTimeStruct.tm_sec = 0;
  time_t lightStartTimestamp = mktime(&lightStartTimeStruct);

  struct tm lightEndTimeStruct = timeinfo;

  char str1[5];
  strncpy(str1, lightEndTime, 2);
  lightEndTimeStruct.tm_hour = atoi(str1);
  strcpy(str1, lightEndTime);
  lightEndTimeStruct.tm_min = atoi(lightEndTime + 2);
  lightEndTimeStruct.tm_sec = 0;
  time_t lightEndTimestamp = mktime(&lightEndTimeStruct);

  // Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  // Serial.println(&lightStartTimeStruct, "%A, %B %d %Y %H:%M:%S");
  // Serial.println(&lightEndTimeStruct, "%A, %B %d %Y %H:%M:%S");

  if (now >= lightStartTimestamp && now < lightEndTimestamp)
  {
    Serial.println("It is time");
    digitalWrite(lightPin, HIGH);
  }
  else
  {
    Serial.println("It is not time");
    digitalWrite(lightPin, LOW);
  }
}

// -------------------------- LOOP ------------------------ //

void loop()
{
  runPump();
  delay(1000);
  runLight();
}
