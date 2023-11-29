#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "../include/secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include "Adafruit_SHT4x.h"
#include <hp_BH1750.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "helper.h"

// --------------- AWS setup ------------------------------//

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "sensor/"
#define AWS_IOT_SUBSCRIBE_TOPIC "cmd/"
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

StaticJsonDocument<200> doc;
char jsonBuffer[512];

// --------------- ESP setup -------------------------//
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
hp_BH1750 BH1750;

#define waterPin 5
#define nutrientPin 13
#define lightPin 14

uint32_t waterPrevMillis;
char *oldWaterStartTime = "120000"; // HHMMSS
char *newWaterStartTime = "120000";
bool startWater = false;
int waterOnDuration = 5000;  // how long to turn on
int waterOffDuration = 2000; // how long to turn off
bool waterIsOff = true;

uint32_t nutrientPrevMillis;
char *oldNutrientStartTime = "120000"; // HHMMSS
char *newNutrientStartTime = "120000";
bool startNutrient = false;
int nutrientOnDuration = 3000;  // how long to turn on
int nutrientOffDuration = 1000; // how long to turn off
bool nutrientIsOff = true;

uint32_t lightPrevMillis;
char *oldLightStartTime = "160000"; // HHMMSS
char *newLightStartTime = "160000";
bool startLight = false;
int lightOnDuration = 28800000;
int lightOffDuration = 57600000;
bool lightIsOff = true;

// -------------- Time setup ----------------------- //
const char *ntpServer = "pool.ntp.org";
// CONSTANTS FOR TIME OFFSET
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;
// -------------- Sensor setup ----------------------//
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
//   if (!sht4.begin())
//   {
//     Serial.println("SHT4x sensor not setup or disconnected!");
//     while (1)
//       delay(1);
//   }

//   sensors_event_t humidity, temp;
//   sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
//   return {temp.temperature, humidity.relative_humidity};
// }

// void setupBH1750()
// {
//   bool avail = BH1750.begin(BH1750_TO_GROUND); // BH1750_TO_GROUND or BH1750_TO_VCC for addr pin
//   if (!avail)
//   {
//     Serial.println("No BH1750 sensor found!");
//     while (1)
//       delay(1);
//   }

//   // BH1750.calibrateTiming();  //uncomment this line if you want to speed up your sensor
//   Serial.printf("conversion time: %dms\n", BH1750.getMtregTime());
//   BH1750.start(); // start the first measurement in setup
// }

// float getLuminance()
// {
//   BH1750.start();
//   float lux = BH1750.getLux();
//   return lux;
// }

// -------------------- AWS functions ----------------//
void messageHandler(String &topic, String &payload)
{
  const size_t capacity = JSON_OBJECT_SIZE(5) + payload.length();

  // Allocate a buffer for the JSON document
  DynamicJsonDocument doc(capacity);

  // Parse the JSON string
  DeserializationError error = deserializeJson(doc, payload);

  // Check for parsing errors
  if (error)
  {
    Serial.print(F("Error parsing JSON: "));
    Serial.println(error.c_str());
    return;
  }

  // Access the inner JSON string
  const char *innerJson = doc["msg"];

  // Deserialize the inner JSON string
  error = deserializeJson(doc, innerJson);

  // Check for parsing errors
  if (error)
  {
    Serial.print(F("Error parsing inner JSON: "));
    Serial.println(error.c_str());
    return;
  }

  String target = doc["target"];
  String start = doc["startTime"];
  String onDuration = doc["onInterval"];
  String offDuration = doc["offInterval"];

  char *startTime = convertToChar(start.c_str());
  int onInterval = convertToMillis(onDuration.c_str());
  int offInterval = convertToMillis(offDuration.c_str());

  if (strcmp(target.c_str(), "water_pump") == 0)
  {
    Serial.println("Modified water pump schedule");
    newWaterStartTime = startTime;
    waterOnDuration = onInterval;
    waterOffDuration = offInterval;
  }
  else if (strcmp(target.c_str(), "nutrient_pump") == 0)
  {
    Serial.println("Modified nutrient pump schedule");
    newNutrientStartTime = startTime;
    nutrientOnDuration = onInterval;
    nutrientOffDuration = offInterval;
  }
  else
  {
    Serial.println("Modified light schedule");
    newLightStartTime = startTime;
    lightOnDuration = onInterval;
    lightOffDuration = offInterval;
  }
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }

  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

// void publishMessage(String sensorType, StaticJsonDocument<200> doc, char *payload)
// {
//   if (sensorType == "temperature")
//   {
//     TempHumidReading tempHumidReading = getTempHumid();
//     doc["value"] = tempHumidReading.first;
//     serializeJson(doc, jsonBuffer);
//     client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("temperature"), jsonBuffer);
//     Serial.println(doc.as<String>());
//   }
//   else if (sensorType == "humidity")
//   {
//     TempHumidReading tempHumidReading = getTempHumid();
//     doc["value"] = tempHumidReading.second;
//     serializeJson(doc, jsonBuffer);
//     client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("humidity"), jsonBuffer);
//     Serial.println(doc.as<String>());
//   }
//   else if (sensorType == "luminance")
//   {
//     float lux = getLuminance();
//     doc["value"] = lux;
//     serializeJson(doc, jsonBuffer);
//     client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("luminance"), jsonBuffer);
//     Serial.println(doc.as<String>());
//   }
// }

// -------------------- ESP control functions ------------ //
void setup()
{
  Serial.begin(115200);
  // setupSHT40();
  // setupBH1750();
  pinMode(waterPin, OUTPUT);
  pinMode(nutrientPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  connectAWS();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void runWaterMillis()
{
  int waterCrntMillis = millis();

  if (waterIsOff)
  {

    if (waterCrntMillis - waterPrevMillis >= waterOffDuration)
    {
      waterIsOff = false;
      waterPrevMillis = waterCrntMillis;
      digitalWrite(waterPin, HIGH);
    }
  }
  else if (waterCrntMillis - waterPrevMillis >= waterOnDuration)
  {
    waterIsOff = true;
    waterPrevMillis = waterCrntMillis;
    digitalWrite(waterPin, LOW);
  }
}

void runNutrientMilis()
{
  int nutrientCrntMillis = millis();

  if (nutrientIsOff)
  {

    if (nutrientCrntMillis - nutrientPrevMillis >= nutrientOffDuration)
    {
      nutrientIsOff = false;
      nutrientPrevMillis = nutrientCrntMillis;
      digitalWrite(nutrientPin, HIGH);
    }
  }
  else if (nutrientCrntMillis - nutrientPrevMillis >= nutrientOnDuration)
  {
    nutrientIsOff = true;
    nutrientPrevMillis = nutrientCrntMillis;
    digitalWrite(nutrientPin, LOW);
  }
}

void runLightMillis()
{
  int lightCrntMillis = millis();

  if (lightIsOff)
  {

    if (lightCrntMillis - lightPrevMillis >= lightOffDuration)
    {
      lightIsOff = false;
      lightPrevMillis = lightCrntMillis;
      digitalWrite(lightPin, HIGH);
    }
  }
  else if (lightCrntMillis - lightPrevMillis >= lightOnDuration)
  {
    lightIsOff = true;
    lightPrevMillis = lightCrntMillis;
    digitalWrite(lightPin, LOW);
  }
}

void runWater()
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
  struct tm waterStartTimeStruct = timeinfo;

  // Convert HHMMSS format to hour and minutes
  char str[7];
  strncpy(str, oldWaterStartTime, 6);
  str[6] = '\0'; // Null-terminate the string
  waterStartTimeStruct.tm_hour = atoi(str);
  waterStartTimeStruct.tm_min = atoi(str + 2);
  waterStartTimeStruct.tm_sec = atoi(str + 4);
  time_t waterStartTimeStamp = mktime(&waterStartTimeStruct);

  if (strcmp(newWaterStartTime, oldWaterStartTime) != 0)
  {
    bool startWater = false;
    oldWaterStartTime = newWaterStartTime;
  }
  else if (now == waterStartTimeStamp)
  {
    bool startWater = true;
  }

  if (startWater)
  {
    runWaterMillis();
  }
  else
  {
    digitalWrite(waterPin, LOW);
  }
}

void runNutrient()
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
  struct tm nutrientStartTimeStruct = timeinfo;

  // Convert HHMMSS format to hour and minutes
  char str[7];
  strncpy(str, oldNutrientStartTime, 6);
  str[6] = '\0'; // Null-terminate the string
  nutrientStartTimeStruct.tm_hour = atoi(str);
  nutrientStartTimeStruct.tm_min = atoi(str + 2);
  nutrientStartTimeStruct.tm_sec = atoi(str + 4);
  time_t nutrientStartTimeStamp = mktime(&nutrientStartTimeStruct);

  if (strcmp(newNutrientStartTime, oldNutrientStartTime) != 0)
  {
    bool startNutrient = false;
    oldNutrientStartTime = newNutrientStartTime;
  }
  else if (now == nutrientStartTimeStamp)
  {
    bool startNutrient = true;
  }

  if (startNutrient)
  {
    runNutrientMilis();
  }
  else
  {
    digitalWrite(nutrientPin, LOW);
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

  // Convert HHMMSS format to hour and minutes
  char str[7];
  strncpy(str, oldWaterStartTime, 6);
  str[6] = '\0'; // Null-terminate the string
  lightStartTimeStruct.tm_hour = atoi(str);
  lightStartTimeStruct.tm_min = atoi(str + 2);
  lightStartTimeStruct.tm_sec = atoi(str + 4);
  time_t lightStartTimeStamp = mktime(&lightStartTimeStruct);

  if (strcmp(newLightStartTime, oldLightStartTime) != 0)
  {
    bool startLight = false;
    oldLightStartTime = newLightStartTime;
  }
  else if (now == lightStartTimeStamp)
  {
    bool startLight = true;
  }

  if (startLight)
  {
    runLightMillis();
  }
  else
  {
    digitalWrite(lightPin, LOW);
  }
}
// -------------------------- LOOP ------------------------ //

void loop()
{
  client.loop();
  delay(100);
  // runWater();
  // runNutrient();
  // delay(1000);
  // printSensor();
  // runLight();
}
