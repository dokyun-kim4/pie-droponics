#include "helper.h"
int convertToMillis(const char *timeString)
{
    /*
    Given a timeString "HR:MN:SC", convert to milliseconds
    */
    int totalMillis = 0;

    // Make a copy of the input string
    char *mutableString = strdup(timeString);

    char *hours = strtok(mutableString, ":");
    char *minutes = strtok(NULL, ":");
    char *seconds = strtok(NULL, ":");

    // Convert substrings to integers and then to milliseconds
    int hoursInt = atoi(hours) * 60 * 60 * 1000;
    int minutesInt = atoi(minutes) * 60 * 1000;
    int secondsInt = atoi(seconds) * 1000;

    totalMillis = hoursInt + minutesInt + secondsInt;

    // Don't forget to free the allocated memory
    free(mutableString);

    return totalMillis;
}
