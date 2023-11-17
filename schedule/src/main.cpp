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
int waterOnDuration = 5000;  // how long to turn on
int waterOffDuration = 2000; // how long to turn off
bool waterIsOff = true;

uint32_t nutrientPrevMillis;
int nutrientOnDuration = 3000;  // how long to turn on
int nutrientOffDuration = 1000; // how long to turn off
bool nutrientIsOff = true;

const char *lightStartTime = "1544";
const char *lightEndTime = "1545";

// -------------- Time setup ----------------------- //
const char *ntpServer = "pool.ntp.org";
// CONSTANTS FOR TIME OFFSET
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;

// -------------------- AWS functions ----------------//
void setupSHT40()
{
  // Setup for temp/humidity sensor
  if (!sht4.begin())
  {
    Serial.println("Couldn't find SHT4x sensor");
    while (1)
      delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);

  // You can have 6 different heater settings
  sht4.setHeater(SHT4X_NO_HEATER);
}

// Create an alias for temperature and humidity readings
typedef std::pair<float, float> TempHumidReading;

// Function to get temperature and humidity readings
TempHumidReading getTempHumid()
{
  if (!sht4.begin())
  {
    Serial.println("SHT4x sensor not setup or disconnected!");
    while (1)
      delay(1);
  }

  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
  return {temp.temperature, humidity.relative_humidity};
}

void setupBH1750()
{
  bool avail = BH1750.begin(BH1750_TO_GROUND); // BH1750_TO_GROUND or BH1750_TO_VCC for addr pin
  if (!avail)
  {
    Serial.println("No BH1750 sensor found!");
    while (1)
      delay(1);
  }

  // BH1750.calibrateTiming();  //uncomment this line if you want to speed up your sensor
  Serial.printf("conversion time: %dms\n", BH1750.getMtregTime());
  BH1750.start(); // start the first measurement in setup
}

float getLuminance()
{
  BH1750.start();
  float lux = BH1750.getLux();
  return lux;
}

void messageHandler(String &topic, String &payload)
{
  // TODO implement this function
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

void publishMessage(String sensorType, StaticJsonDocument<200> doc, char *payload)
{
  if (sensorType == "temperature")
  {
    TempHumidReading tempHumidReading = getTempHumid();
    doc["value"] = tempHumidReading.first;
    serializeJson(doc, jsonBuffer);
    client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("temperature"), jsonBuffer);
    Serial.println(doc.as<String>());
  }
  else if (sensorType == "humidity")
  {
    TempHumidReading tempHumidReading = getTempHumid();
    doc["value"] = tempHumidReading.second;
    serializeJson(doc, jsonBuffer);
    client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("humidity"), jsonBuffer);
    Serial.println(doc.as<String>());
  }
  else if (sensorType == "luminance")
  {
    float lux = getLuminance();
    doc["value"] = lux;
    serializeJson(doc, jsonBuffer);
    client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("luminance"), jsonBuffer);
    Serial.println(doc.as<String>());
  }
}

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
  setupSHT40();
  setupBH1750();
  pinMode(waterPin, OUTPUT);
  pinMode(nutrientPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  connectWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void runWater()
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

void runNutrient()
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
    // Serial.println("It is time");
    digitalWrite(lightPin, HIGH);
  }
  else
  {
    // Serial.println("It is not time");
    digitalWrite(lightPin, LOW);
  }
}

void printSensor()
{
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data
  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degrees C");
  Serial.print("Humidity: ");
  Serial.print(humidity.relative_humidity);
  Serial.println("% rH");
  float lum = getLuminance();
  Serial.print("Luminance: ");
  Serial.println(lum);
}
// -------------------------- LOOP ------------------------ //

void loop()
{
  runWater();
  runNutrient();
  delay(1000);
  printSensor();
  runLight();
}
