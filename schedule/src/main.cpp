#include <Arduino.h>

// put function declarations here:
#define ledPin 45
int onDelay = 2000; // millisecond
int offDelay = 5000;
bool isOn = false;

uint32_t prevMillis;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  prevMillis = millis();
}

void inputSchedule()
{
  Serial.println("Enter light schedule (sec)");
  int schedule = Serial.parseInt(); // in seconds
  if (schedule != 0)
  {
    offDelay = 1000 * schedule;
    Serial.printf("New delay: %d seconds \n", schedule);
  }
}

void loop()
{
  if (Serial.available() > 0)
  {
    inputSchedule();
  }

  uint32_t t;
  int currentMillis = millis();

  if (!isOn)
  {
    if (currentMillis - prevMillis >= offDelay)
    {
      prevMillis = currentMillis;
      isOn = true;
      neopixelWrite(ledPin, 255, 0, 0);
    }
  }
  else if (currentMillis - prevMillis >= onDelay)
  {
    prevMillis = currentMillis;
    isOn = false;
    neopixelWrite(ledPin, 0, 0, 0);
  }
}
