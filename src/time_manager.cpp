#include "time_manager.h"
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
RTC_DS3231 TimeManager::rtc;
NTPClient TimeManager::timeClient(ntpUDP, "pool.ntp.org", UTC_OFFSET * 3600);
Dusk2Dawn TimeManager::d2d(latitude, longitude, UTC_OFFSET);

// Variables statiques pour stocker les horaires
int TimeManager::sunriseMinutes = 0;
int TimeManager::sunsetMinutes = 0;

void TimeManager::init() {
    if (!rtc.begin()) {
        Serial.println("RTC non trouvé");
        return;
    }

    if (rtc.lostPower()) {
        Serial.println("RTC a perdu son alimentation, initialisation avec l'heure NTP");
        updateRTCTime();
    }

    timeClient.begin();
    updateRTCTime();
    calculateSunriseSunset();
}

void TimeManager::calculateSunriseSunset() {
    DateTime now = rtc.now();
    
    // Calcul des horaires en minutes depuis minuit
    sunriseMinutes = d2d.sunrise(now.year(), now.month(), now.day(), false);
    sunsetMinutes = d2d.sunset(now.year(), now.month(), now.day(), false);
    
    // Ajustement pour l'heure d'été
    if (isDST(now)) {
        sunriseMinutes += 60;
        sunsetMinutes += 60;
    }
    
    // Affichage des horaires (pour debug)
    char sunriseTime[6], sunsetTime[6];
    formatTime(sunriseMinutes, sunriseTime);
    formatTime(sunsetMinutes, sunsetTime);
    
    Serial.print("Lever du soleil: ");
    Serial.println(sunriseTime);
    Serial.print("Coucher du soleil: ");
    Serial.println(sunsetTime);
}

bool TimeManager::isDST(DateTime date) {
    if (date.month() < DST_START_MONTH || date.month() > DST_END_MONTH) {
        return false;
    }
    if (date.month() > DST_START_MONTH && date.month() < DST_END_MONTH) {
        return true;
    }
    
    int lastSunday = DST_CHANGE_DAY - date.dayOfTheWeek();
    
    if (date.month() == DST_START_MONTH) {
        return date.day() >= lastSunday;
    } else {
        return date.day() < lastSunday;
    }
}

// Getters pour les horaires
int TimeManager::getSunriseMinutes() {
    return sunriseMinutes;
}

int TimeManager::getSunsetMinutes() {
    return sunsetMinutes;
}

// Méthode pour obtenir l'heure actuelle en minutes depuis minuit
int TimeManager::getCurrentTimeMinutes() {
    DateTime now = rtc.now();
    return now.hour() * 60 + now.minute();
}

bool TimeManager::updateRTCTime() {
    if (timeClient.update()) {
        time_t epochTime = timeClient.getEpochTime();
        rtc.adjust(DateTime(epochTime));
        return true;
    }
    return false;
}

void TimeManager::updateTimeTask(void *parameter) {
    for(;;) {
        updateRTCTime();
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Mise à jour toutes les heures
    }
}

void TimeManager::calculateSunriseSunsetTask(void *parameter) {
    for(;;) {
        calculateSunriseSunset();
        vTaskDelay(pdMS_TO_TICKS(86400000)); // Calcul une fois par jour
    }
}

void TimeManager::formatTime(int timeInMinutes, char *buffer) {
    int hours = timeInMinutes / 60;
    int minutes = timeInMinutes % 60;
    sprintf(buffer, "%02d:%02d", hours, minutes);
}