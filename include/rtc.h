#ifndef RTC_H
#define RTC_H

#include <RTClib.h>
#include <Timezone.h>
#include <Dusk2Dawn.h>

bool updateDS3231Time();
void calculateSunriseSunset();
void calculateSunriseSunsetTask(void *pvParameters);

#endif // RTC_H
