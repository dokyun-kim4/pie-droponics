#include <Arduino.h>
#include "Adafruit_SHT4x.h"
#define pumpPin 10

void getTempHumid();
void schedulePump();

Adafruit_SHT4x sht4 = Adafruit_SHT4x();

uint32_t prevMillis;
int onDuration = 2000;  // how long to turn on
int offDuration = 5000; // how long to turn off

bool isOn = false;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  
  // Setup for pump schedule
  pinMode(pumpPin, OUTPUT);
  prevMillis = millis();

  // Setup for temp/humidity sensor
  if (! sht4.begin()) 
  {
    Serial.println("Couldn't find SHT4x sensor");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);

  // You can have 6 different heater settings
  sht4.setHeater(SHT4X_NO_HEATER);
}

void loop()
{
  if (Serial.available())
  {
    schedulePump();
  }

  uint32_t t;
  t = millis();
  int currentMillis = millis();

  if (!isOn)
  {
    if (currentMillis - prevMillis >= offDuration)
    {
      prevMillis = currentMillis;
      isOn = true;
      digitalWrite(pumpPin, HIGH);
      getTempHumid();
    }
  }
  else if (currentMillis - prevMillis >= onDuration)
  {
    prevMillis = currentMillis;
    isOn = false;
    digitalWrite(pumpPin, LOW);
  }
}

void schedulePump()
{
  int schedule = Serial.parseInt(); // in seconds
  if (schedule != 0)
  {
    offDuration = 1000 * schedule; // convert to ms
    Serial.printf("New pump delay: %d seconds \n", schedule);
  }
}

void getTempHumid()
{
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  Serial.print("Temperature: "); 
  Serial.print(temp.temperature); 
  Serial.println(" degrees C");
  Serial.print("Humidity: "); 
  Serial.print(humidity.relative_humidity); 
  Serial.println("% rH");
}