#include "actuators.h"

void updateScheduleState(DeviceSchedule &device)
{
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    time_t currentTimeMillis = mktime(&timeinfo);

    if (device.currentState == IDLE && device.nextActuationTime <= currentTimeMillis)
    {
        device.currentState = ON_DURATION;
        device.nextActuationTime = currentTimeMillis + device.onDurationSeconds;
    }
    else if (device.currentState == ON_DURATION && device.nextActuationTime <= currentTimeMillis)
    {
        device.currentState = OFF_DURATION;
        device.nextActuationTime = currentTimeMillis + device.offDurationSeconds;
    }
    else if (device.currentState == OFF_DURATION && device.nextActuationTime <= currentTimeMillis)
    {
        device.currentState = ON_DURATION;
        device.nextActuationTime = currentTimeMillis + device.onDurationSeconds;
    }
}

void performScheduledAction(DeviceSchedule &device, std::map<const char *, int> CONTROL_PINS)
{
    int controlPin = CONTROL_PINS[device.target]; // Find the pin number based on the target device
    switch (device.currentState)
    {
    case ON_DURATION:
        Serial.println("ON");
        Serial.println(device.target);
        digitalWrite(controlPin, HIGH);
        break;
    case OFF_DURATION:
        digitalWrite(controlPin, LOW);
        break;
    default:
        digitalWrite(controlPin, LOW); // Default state (IDLE)
        break;
    }
}

time_t midnightEpochTime()
{
    // Get current time
    time_t now;
    struct tm *timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    // Set time to midnight
    timeinfo->tm_hour = 0;
    timeinfo->tm_min = 0;
    timeinfo->tm_sec = 0;

    // Convert struct tm back to time_t
    return mktime(timeinfo);
}
