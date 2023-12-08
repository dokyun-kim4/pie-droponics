#include <TimeLib.h>
#include <Arduino.h>
#include <map>
enum ScheduleState
{
    IDLE,
    ON_DURATION,
    OFF_DURATION
};

struct DeviceSchedule
{
    ScheduleState currentState;
    unsigned long nextActuationTime;
    unsigned long onDurationSeconds;
    unsigned long offDurationSeconds;
    const char *target; // Assuming each device has a unique identifier
};
extern DeviceSchedule devices[];
void performScheduledAction(DeviceSchedule &device, std::map<const char *, int> CONTROL_PINS);
void updateScheduleState(DeviceSchedule &device);
time_t midnightEpochTime();