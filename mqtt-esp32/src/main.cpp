#include <Arduino.h>
#include "../include/secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "Adafruit_SHT4x.h"

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC "sensor/"
#define AWS_IOT_SUBSCRIBE_TOPIC "cmd/"

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);
int id = 0;

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

void publishMessage()
{
  StaticJsonDocument<200> doc;
  TempHumidReading tempHumidReading = getTempHumid();
  doc["value"] = tempHumidReading.first;
  // doc["msg"]["humidity"] = tempHumidReading.second;

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client
  client.publish(String(AWS_IOT_PUBLISH_TOPIC) + String("temperature"), jsonBuffer);
  Serial.println(doc.as<String>());
}

void setup()
{
  Serial.begin(9600);
  connectAWS();
  setupSHT40();
}

void loop()
{
  publishMessage();
  client.loop();
  delay(5000);
}
