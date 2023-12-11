// helper.h
#include <Arduino.h>
#ifndef HELPER_H
#define HELPER_H

// Function to convert time string to milliseconds
int convertToMillis(const char *durationTime);

// Structure to represent time
struct timeStruct
{
    int hr;
    int min;
    int sec;
};

// Function to convert time string to time struct
char *convertToChar(const char *startTime);

#endif // HELPER_H
