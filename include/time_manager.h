#pragma once

#include <Arduino.h>
#include <time.h>
#include "config.h"

struct TimeData
{
    int hour;
    int minute;
    int second;
    int day;
    int month;
    int year;
    int weekday;
    bool timeValid;
};

class TimeManager
{
public:
    TimeManager();

    void init();
    void update();
    void setTime(int hour, int minute, int second, int day, int month, int year);

    TimeData getTimeData();

    // Helper functions
    String getFormattedTime();
    String getFormattedDate();
    String getWeekdayName();
    String getMonthName();

private:
    TimeData _timeData;
    unsigned long _lastUpdate;
    void syncWithNTP();
};