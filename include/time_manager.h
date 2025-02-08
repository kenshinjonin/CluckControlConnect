#include <NTPClient.h>
#include <RTClib.h>
#include <Timezone.h>
#include <Dusk2Dawn.h>
#include "config.h"

class TimeManager {
public:
    static void init();
    static void calculateSunriseSunset();
    static bool updateRTCTime();
    static void updateTimeTask(void *parameter);
    static void calculateSunriseSunsetTask(void *parameter);
    
    // Nouvelles méthodes
    static int getSunriseMinutes();
    static int getSunsetMinutes();
    static int getCurrentTimeMinutes();
    
private:
    static RTC_DS3231 rtc;
    static NTPClient timeClient;
    static Dusk2Dawn d2d;
    
    static int sunriseMinutes;
    static int sunsetMinutes;
    
    static void formatTime(int time, char *buffer);
    static bool isDST(DateTime date);
}; 