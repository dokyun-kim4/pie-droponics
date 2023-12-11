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
#include "actuators.h"
#include <map>

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

std::map<const char *, int> CONTROL_PINS = {
    {"nutrient_pump", 13},
    {"led", 14},
    {"water_pump", 15}};

// --------------- Actuator setup -------------------//
const int NUM_DEVICES = 3;

// seconds since epoch of midnight of current day (00:00 AM of current day)
unsigned long tmrwMidnightTime = midnightEpochTime() + 24 * 3600;                       // seconds since epoch of midnight of previous day
DeviceSchedule nutrient_pump = {IDLE, tmrwMidnightTime, 0, 24 * 3600, "nutrient_pump"}; // 0 hours on, 24 hours off
DeviceSchedule led = {IDLE, midnightEpochTime(), 3 * 3600, 3 * 3600, "led"};            // 3 hours on, 3 hours off
DeviceSchedule water_pump = {IDLE, tmrwMidnightTime, 0, 24 * 3600, "water_pump"};       // 0 hours on, 24 hours off

// Combine all the actuators into an array
DeviceSchedule actuators[] = {nutrient_pump, led, water_pump};

// -------------- Time setup ----------------------- //
const char *ntpServer = "pool.ntp.org";
// CONSTANTS FOR TIME OFFSET
const long gmtOffset_sec = -18000;
const int daylightOffset_sec = 3600;
// -------------- Sensor setup ----------------------//
void setupSHT40()
{
  if (!sht4.begin())
  {
    Serial.println("Couldn't find SHT4x sensor");
    while (1)
      delay(1);
  }
  delay(100);
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  sht4.setPrecision(SHT4X_HIGH_PRECISION);
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

// -------------------- AWS functions ----------------//
void messageHandler(String &topic, String &payload)
{
  const size_t capacity = JSON_OBJECT_SIZE(5) + payload.length();
  Serial.println("Message arrived on topic: " + topic);
  Serial.println("Message:");
  Serial.println(payload);
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

  const char *target = doc["target"];
  // Find the device index based on the target
  int deviceIndex = -1;
  for (int i = 0; i < NUM_DEVICES; ++i)
  {
    if (strcmp(actuators[i].target, target) == 0)
    {
      deviceIndex = i;
      Serial.print("Cmd target device found: ");
      Serial.println(target);
      break;
    }
  }

  struct tm timeinfo;
  getLocalTime(&timeinfo);
  time_t currentTimeMillis = mktime(&timeinfo); // seconds since epoch
  time_t midnightEpoch = midnightEpochTime();   // seconds since epoch of midnight of current day

  // If the device is found, update its scheduling information
  if (deviceIndex != -1)
  {
    const char *start = doc["startTime"];
    Serial.println(start);

    const char *onDuration = doc["onInterval"];
    Serial.println(onDuration);

    const char *offDuration = doc["offInterval"];
    Serial.println(offDuration);

    int hours, minutes, seconds;

    struct tm tmTime;
    sscanf(start, "%d:%d:%d", &hours, &minutes, &seconds);

    unsigned long startTime = hours * 3600 + minutes * 60 + seconds;
    Serial.println(startTime);

    startTime += midnightEpoch;
    Serial.println(startTime);

    sscanf(onDuration, "%d:%d:%d", &hours, &minutes, &seconds);
    actuators[deviceIndex].onDurationSeconds = hours * 3600 + minutes * 60 + seconds;

    sscanf(offDuration, "%d:%d:%d", &hours, &minutes, &seconds);
    actuators[deviceIndex].offDurationSeconds = hours * 3600 + minutes * 60 + seconds;

    if (currentTimeMillis < startTime)
    {
      actuators[deviceIndex].currentState = IDLE;
      actuators[deviceIndex].nextActuationTime = startTime;
    }
    else
    {
      actuators[deviceIndex].currentState = ON_DURATION;
      actuators[deviceIndex].nextActuationTime = currentTimeMillis + actuators[deviceIndex].onDurationSeconds;
    }
  }

  Serial.println("Updated schedule:");
  Serial.print("Device: ");
  Serial.println(actuators[deviceIndex].target);
  Serial.print("Current state: ");
  Serial.println(actuators[deviceIndex].currentState);
  Serial.print("Next actuation time: ");
  Serial.println(actuators[deviceIndex].nextActuationTime);
  Serial.print("On duration: ");
  Serial.println(actuators[deviceIndex].onDurationSeconds);
  Serial.print("Off duration: ");
  Serial.println(actuators[deviceIndex].offDurationSeconds);

  Serial.println("-----------------------");
  Serial.println("Current time: ");
  Serial.println(currentTimeMillis);
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

void publishMessage(String sensorType)
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
    if (!tempHumidReading.second)
    {
      Serial.println("Retry humidity reading");
      tempHumidReading = getTempHumid();
    }
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
  doc.clear();                               // clear the JSON document for reuse
  memset(jsonBuffer, 0, sizeof(jsonBuffer)); // clear the char array
}

// -------------------- ESP control functions ------------ //
void setup()
{
  Serial.begin(115200);
  delay(1000);
  pinMode(CONTROL_PINS["nutrient_pump"], OUTPUT);
  pinMode(CONTROL_PINS["led"], OUTPUT);
  pinMode(CONTROL_PINS["water_pump"], OUTPUT);
  connectAWS();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  // // Initialize the device schedule
  setupSHT40();
  setupBH1750();
}

// -------------------------- LOOP ------------------------ //
void loop()
{
  static unsigned long lastPublishTime = 0;
  const unsigned long publishInterval = 30000; // 30 seconds in milliseconds

  unsigned long currentMillis = millis();

  // Check if it's time to publish sensor data
  if (currentMillis - lastPublishTime >= publishInterval)
  {
    lastPublishTime = currentMillis;

    String sensorTypes[] = {"temperature", "humidity", "luminance"};
    publishMessage(sensorTypes[0]);
    publishMessage(sensorTypes[1]);
    publishMessage(sensorTypes[2]);
  }

  client.loop();

  for (int i = 0; i < NUM_DEVICES; ++i)
  {
    updateScheduleState(actuators[i]);
    performScheduledAction(actuators[i], CONTROL_PINS);
  }

  delay(10);
}
