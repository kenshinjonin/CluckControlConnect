/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.h
 * --------------
 * Déclaration de la classe TimeManager pour la gestion du temps :
 *   - Initialisation RTC/NTP
 *   - Calcul lever/coucher du soleil
 *   - Décalages configurables appliqués au lever/coucher (persistés en NVS)
 *   - Prise en compte de l'heure d'été (DST)
 *   - Tâches FreeRTOS pour la mise à jour
 */

#pragma once

#include <RTClib.h>
#include <NTPClient.h>
#include <Dusk2Dawn.h>
#include <cmath>

// Bornes des décalages configurables (minutes)
static const int TIME_OFFSET_MIN = -120;
static const int TIME_OFFSET_MAX = 120;

class TimeManager {
public:
    static void init();
    static void calculateSunriseSunset();
    static bool isDST(DateTime date);

    // Retournent -1 quand l'heure ou les éphémérides ne sont pas exploitables
    // (RTC absente, date invalide). L'appelant doit geler l'automatisme.
    static int getSunriseMinutes();
    static int getSunsetMinutes();
    static int getCurrentTimeMinutes();

    // Décalages appliqués au lever/coucher, bornés et conservés en NVS
    static int getSunriseOffset();
    static int getSunsetOffset();
    static void setSunriseOffset(int minutes);
    static void setSunsetOffset(int minutes);

    static bool isRTCAvailable();
    static bool updateRTCTime();
    static void updateTimeTask(void *parameter);
    static void calculateSunriseSunsetTask(void *parameter);
    static void formatTime(int timeInMinutes, char *buffer, size_t size);

    static RTC_DS3231 rtc;
    static NTPClient timeClient;
    static Dusk2Dawn d2d;

private:
    // Horaires astronomiques bruts, décalage DST inclus mais hors décalages
    // utilisateur : ceux-ci sont appliqués dans les accesseurs pour prendre
    // effet immédiatement, sans attendre le recalcul quotidien.
    static volatile int rawSunriseMinutes;
    static volatile int rawSunsetMinutes;
    static volatile int sunriseOffsetMinutes;
    static volatile int sunsetOffsetMinutes;
    static volatile bool rtcAvailable;
};
