#include <Arduino.h>
#include "../include/secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Adafruit_SHT4x.h"
#include <hp_BH1750.h>

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "sensor/"
#define AWS_IOT_SUBSCRIBE_TOPIC "cmd/"

hp_BH1750 BH1750; // Luminance sensor

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

StaticJsonDocument<200> doc;
char jsonBuffer[512];

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
  Serial.println("incoming: " + topic + " - " + payload);
}

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // WiFi.begin("OpenWireless", NULL);

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

void setup()
{
  Serial.begin(115200);
  connectAWS();
  setupSHT40();
  setupBH1750();
}

void loop()
{
  publishMessage(String('temperature'), doc, jsonBuffer);
  doc.clear();                               // clear the JSON document for reuse
  memset(jsonBuffer, 0, sizeof(jsonBuffer)); // clear the char array
  publishMessage(String('humidity'), doc, jsonBuffer);
  doc.clear();                               // clear the JSON document for reuse
  memset(jsonBuffer, 0, sizeof(jsonBuffer)); // clear the char array
  publishMessage(String('luminance'), doc, jsonBuffer);
  doc.clear();                               // clear the JSON document for reuse
  memset(jsonBuffer, 0, sizeof(jsonBuffer)); // clear the char array
  client.loop();

  delay(3000);
}
