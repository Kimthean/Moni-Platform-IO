#include "time_manager.h"

TimeManager::TimeManager()
{
    _lastUpdate = 0;
    _timeData = {0, 0, 0, 1, 1, 2023, 0, false};
}

void TimeManager::init()
{
    // Configure NTP with multiple servers for reliability
    configTzTime("ICT-7", NTP_SERVER, NTP_SERVER_BACKUP);

    // Initial sync
    syncWithNTP();
}

void TimeManager::update()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        _timeData.hour = timeinfo.tm_hour;
        _timeData.minute = timeinfo.tm_min;
        _timeData.second = timeinfo.tm_sec;
        _timeData.day = timeinfo.tm_mday;
        _timeData.month = timeinfo.tm_mon + 1;    // tm_mon is 0-based
        _timeData.year = timeinfo.tm_year + 1900; // tm_year is years since 1900
        _timeData.weekday = timeinfo.tm_wday;
        _timeData.timeValid = true;
    }
}

void TimeManager::syncWithNTP()
{
    Serial0.println("Synchronizing time with NTP server...");
    Serial0.println("Timezone: Cambodia/Phnom Penh (GMT+7)");

    struct tm timeinfo;

    // Try multiple times to get the time
    int retries = 0;
    const int maxRetries = 5;

    while (!getLocalTime(&timeinfo) && retries < maxRetries)
    {
        Serial0.println("Retrying NTP sync...");
        delay(1000);
        retries++;
    }

    if (getLocalTime(&timeinfo))
    {
        _timeData.hour = timeinfo.tm_hour;
        _timeData.minute = timeinfo.tm_min;
        _timeData.second = timeinfo.tm_sec;
        _timeData.day = timeinfo.tm_mday;
        _timeData.month = timeinfo.tm_mon + 1;
        _timeData.year = timeinfo.tm_year + 1900;
        _timeData.weekday = timeinfo.tm_wday;
        _timeData.timeValid = true;

        Serial0.println("Time synchronized successfully");
        Serial0.printf("Cambodia/Phnom Penh: %s %s\n",
                       getFormattedDate().c_str(),
                       getFormattedTime().c_str());
        Serial0.printf("Day of week: %s\n", getWeekdayName().c_str());
    }
    else
    {
        Serial0.println("Failed to obtain time from NTP after multiple attempts");
        _timeData.timeValid = false;
    }
}

void TimeManager::setTime(int hour, int minute, int second, int day, int month, int year)
{
    struct tm timeinfo;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_mday = day;
    timeinfo.tm_mon = month - 1;    // tm_mon is 0-based
    timeinfo.tm_year = year - 1900; // tm_year is years since 1900

    time_t t = mktime(&timeinfo);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);

    _timeData.hour = hour;
    _timeData.minute = minute;
    _timeData.second = second;
    _timeData.day = day;
    _timeData.month = month;
    _timeData.year = year;
    _timeData.timeValid = true;
}

TimeData TimeManager::getTimeData()
{
    return _timeData;
}

String TimeManager::getFormattedTime()
{
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", _timeData.hour, _timeData.minute, _timeData.second);
    return String(buffer);
}

String TimeManager::getFormattedDate()
{
    char buffer[20];
    sprintf(buffer, "%02d %s %04d",
            _timeData.day,
            getMonthName().substring(0, 3).c_str(),
            _timeData.year);
    return String(buffer);
}

String TimeManager::getWeekdayName()
{
    const char *weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    if (_timeData.weekday >= 0 && _timeData.weekday < 7)
    {
        return String(weekdays[_timeData.weekday]);
    }
    return "Unknown";
}

String TimeManager::getMonthName()
{
    const char *months[] = {"January", "February", "March", "April", "May", "June", "July",
                            "August", "September", "October", "November", "December"};
    if (_timeData.month >= 1 && _timeData.month <= 12)
    {
        return String(months[_timeData.month - 1]);
    }
    return "Unknown";
}