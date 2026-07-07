/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.cpp
 * ---------------
 * Gère la synchronisation et la gestion du temps :
 *   - Initialisation RTC/NTP
 *   - Calcul automatique du lever/coucher du soleil (Dusk2Dawn)
 *   - Prise en compte de l'heure d'été (DST)
 *   - Tâches FreeRTOS pour la mise à jour de l'heure et du lever/coucher
 *   - Fournit des getters pour l'heure courante, lever, coucher
 *
 * Convention horaire : la RTC est réglée en heure d'hiver toute l'année
 * (offset NTP fixe). Le décalage d'été (+60 min) est appliqué de façon
 * cohérente à la fois aux horaires de lever/coucher et à l'heure courante
 * retournée par getCurrentTimeMinutes().
 */

#include "config.h"
#include "time_manager.h"
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
RTC_DS3231 TimeManager::rtc;
NTPClient TimeManager::timeClient(ntpUDP, "pool.ntp.org", config.timezoneOffset * 3600); // Décalage UTC en hiver, DST ajouté dynamiquement
Dusk2Dawn TimeManager::d2d(config.latitude, config.longitude, config.timezoneOffset); // Décalage UTC en hiver, DST ajouté dynamiquement

// --- Variables statiques pour stocker les horaires ---
int TimeManager::sunriseMinutes = 0;
int TimeManager::sunsetMinutes = 0;

// --- Initialisation du module temps (RTC/NTP, calcul lever/coucher) ---
void TimeManager::init() {
    if (!rtc.begin()) {
        Serial.println("RTC non trouve");
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

// --- Calcul du lever et coucher du soleil (avec ajustement DST) ---
void TimeManager::calculateSunriseSunset() {
    DateTime now = rtc.now();

    // Calcul des horaires en minutes depuis minuit (heure d'hiver)
    sunriseMinutes = d2d.sunrise(now.year(), now.month(), now.day(), false);
    sunsetMinutes = d2d.sunset(now.year(), now.month(), now.day(), false);

    // Ajustement pour l'heure d'été (getCurrentTimeMinutes applique le même décalage)
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

// --- Détermination si une date est en heure d'été (DST) ---
// Règle européenne approchée : du dernier dimanche du mois de début
// au dernier dimanche du mois de fin (l'heure exacte du changement est ignorée).
bool TimeManager::isDST(DateTime date) {
    if (!config.dstEnabled) return false;
    if (date.month() < config.DST_START_MONTH || date.month() > config.DST_END_MONTH) {
        return false;
    }
    if (date.month() > config.DST_START_MONTH && date.month() < config.DST_END_MONTH) {
        return true;
    }

    // Jour de la semaine du 31 du mois courant (mars et octobre ont 31 jours), 0 = dimanche
    int weekdayOf31 = (date.dayOfTheWeek() + (31 - date.day())) % 7;
    int lastSunday = 31 - weekdayOf31;

    if (date.month() == config.DST_START_MONTH) {
        return date.day() >= lastSunday;
    } else {
        return date.day() < lastSunday;
    }
}

// --- Getters pour les horaires de lever/coucher ---
int TimeManager::getSunriseMinutes() {
    return sunriseMinutes;
}

int TimeManager::getSunsetMinutes() {
    return sunsetMinutes;
}

// --- Obtenir l'heure locale actuelle en minutes depuis minuit ---
// La RTC est en heure d'hiver : on applique le décalage d'été pour rester
// cohérent avec les horaires de lever/coucher ajustés dans calculateSunriseSunset().
int TimeManager::getCurrentTimeMinutes() {
    DateTime now = rtc.now();
    int minutes = now.hour() * 60 + now.minute();
    if (isDST(now)) {
        minutes += 60;
    }
    return minutes % 1440;
}

// --- Mise à jour de l'heure RTC depuis NTP ---
bool TimeManager::updateRTCTime() {
    if (timeClient.update()) {
        time_t epochTime = timeClient.getEpochTime();
        rtc.adjust(DateTime(epochTime));
        return true;
    }
    return false;
}

// --- Tâche FreeRTOS : mise à jour périodique de l'heure RTC ---
void TimeManager::updateTimeTask(void *parameter) {
    for(;;) {
        updateRTCTime();
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Mise à jour toutes les heures
    }
}

// --- Tâche FreeRTOS : calcul quotidien lever/coucher du soleil ---
void TimeManager::calculateSunriseSunsetTask(void *parameter) {
    for(;;) {
        calculateSunriseSunset();
        vTaskDelay(pdMS_TO_TICKS(86400000)); // Calcul une fois par jour
    }
}

// --- Formatage d'un horaire (minutes -> HH:MM) ---
void TimeManager::formatTime(int timeInMinutes, char *buffer) {
    int hours = timeInMinutes / 60;
    int minutes = timeInMinutes % 60;
    sprintf(buffer, "%02d:%02d", hours, minutes);
}
