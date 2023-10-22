#include <Arduino.h>

// put function declarations here:
const int ledPin = 5;
int delayTime = 1000; // millisecond

uint32_t blinkTime;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  blinkTime = millis();
}

void inputSchedule()
{
  Serial.println("Enter light schedule (sec)");
  int schedule = Serial.parseInt(); // in seconds
  if (schedule != 0)
  {
    delayTime = 1000 * schedule;
    Serial.println(delayTime);
  }
}

void loop()
{
  if (Serial.available() > 0)
  {
    inputSchedule();
  }

  uint32_t t;
  t = millis();

  if (t >= blinkTime + delayTime)
  {
    digitalWrite(ledPin, !digitalRead(ledPin));
    blinkTime = t;
  }
}
