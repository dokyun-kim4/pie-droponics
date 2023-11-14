#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include <Arduino.h>
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
#define pumpPin 10
#define lightPin 11

uint32_t pumpPrevMillis;
char *pumpStartTime = "1100"; // HHMM format
int pumpOnDuration = 1000;    // how long to turn on
int pumpOffDuration = 2000;   // how long to turn off
bool pumpIsOn = false;

uint32_t lightPrevMillis;
char *lightStartTime = "1100";
int lightOnDuration = 5000;
int lightOffDuration = 10000;
bool lightIsOn = false;

// AWS functions

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
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp); // Populate temp and humidity objects with fresh data
  return {temp.temperature, humidity.relative_humidity};
}

void messageHandler(String &topic, String &payload)
{
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char *target = doc["target"];
  const char *startTime = doc["start_time"];
  const char *onDuration = doc["on_interval"];
  const char *offDuration = doc["off_interval"];

  if (strcmp(target, "pump") == 0)
  {
    pumpStartTime = startTime;
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

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  TempHumidReading tempHumidReading = getTempHumid();
  doc["temp"] = tempHumidReading.first;
  doc["humidity"] = tempHumidReading.second;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
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
  pinMode(pumpPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pumpPrevMillis = millis();
  lightPrevMillis = millis()
}

void runPump()
{
  uint32_t t;
  t = millis();
  int currentMillis = millis();

  if (!pumpIsOn)
  {
    if (currentMillis - pumpPrevMillis >= pumpOffDuration)
    {
      lightPrevMillis = currentMillis;
      pumpIsOn = true;
      digitalWrite(pumpPin, HIGH);
    }
  }
  else if (currentMillis - pumpPrevMillis >= pumpOnDuration)
  {
    pumpPrevMillis = currentMillis;
    pumpIsOn = false;
    digitalWrite(pumpPin, LOW);
  }
}

void runLight()
{
  uint32_t t;
  t = millis();
  int currentMillis = millis();

  if (!pumpIsOn)
  {
    if (currentMillis - lightPrevMillis >= lightOffDuration)
    {
      lightPrevMillis = currentMillis;
      lightIsOn = true;
      digitalWrite(lightPin, HIGH);
    }
  }
  else if (currentMillis - lightPrevMillis >= lightOffDuration)
  {
    lightPrevMillis = currentMillis;
    lightIsOn = false;
    digitalWrite(pumpPin, LOW);
  }
}

void loop()
{
  runLight();
  runPump();
}