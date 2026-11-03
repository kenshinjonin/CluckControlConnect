/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.h
 * --------------
 * Déclaration de la classe TimeManager pour la gestion du temps :
 *   - Horloge système synchronisée par NTP, RTC DS3231 en secours
 *   - Fuseau horaire et heure d'été gérés nativement (règle POSIX)
 *   - Calcul lever/coucher du soleil
 *   - Décalages configurables appliqués au lever/coucher (persistés en NVS)
 *   - Tâches FreeRTOS pour la mise à jour
 */

#pragma once

#include <Arduino.h>

// Bornes des décalages configurables (minutes)
static const int TIME_OFFSET_MIN = -120;
static const int TIME_OFFSET_MAX = 120;

class TimeManager {
public:
    static void init();

    // Retournent -1 quand l'heure ou les éphémérides ne sont pas exploitables
    // (ni NTP ni RTC, date invalide). L'appelant doit geler l'automatisme.
    static int getSunriseMinutes();
    static int getSunsetMinutes();
    static int getCurrentTimeMinutes();

    // Décalages appliqués au lever/coucher, bornés et conservés en NVS
    static int getSunriseOffset();
    static int getSunsetOffset();
    static void setSunriseOffset(int minutes);
    static void setSunsetOffset(int minutes);

    static void updateTimeTask(void *parameter);
    static void calculateSunriseSunsetTask(void *parameter);
    static void formatTime(int timeInMinutes, char *buffer, size_t size);

private:
    static void calculateSunriseSunset();

    // Horaires astronomiques bruts en heure locale, hors décalages utilisateur :
    // ceux-ci sont appliqués dans les accesseurs pour prendre effet
    // immédiatement, sans attendre le recalcul.
    static volatile int rawSunriseMinutes;
    static volatile int rawSunsetMinutes;
    static volatile int sunriseOffsetMinutes;
    static volatile int sunsetOffsetMinutes;
    static volatile bool rtcAvailable;
};
