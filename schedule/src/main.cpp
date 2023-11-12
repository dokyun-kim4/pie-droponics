#include <Arduino.h>
#define pumpPin 10

uint32_t prevMillis;
int onDuration = 2000;  // how long to turn on
int offDuration = 5000; // how long to turn off

bool isOn = false;

void setup()
{
  Serial.begin(115200);
  pinMode(pumpPin, OUTPUT);
  prevMillis = millis();
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

void runPump()
{
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
    }
  }
  else if (currentMillis - prevMillis >= onDuration)
  {
    prevMillis = currentMillis;
    isOn = false;
    digitalWrite(pumpPin, LOW);
  }
}

void loop()
{
  if (Serial.available() > 0)
  {
    schedulePump();
  }
  runPump();
}