/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.h
 * --------------
 * Déclaration de la classe TimeManager pour la gestion du temps :
 *   - Initialisation RTC/NTP
 *   - Calcul lever/coucher du soleil
 *   - Prise en compte de l'heure d'été (DST)
 *   - Tâches FreeRTOS pour la mise à jour
 */

#pragma once

#include <RTClib.h>
#include <NTPClient.h>
#include <Dusk2Dawn.h>

class TimeManager {
public:
    static void init();
    static void calculateSunriseSunset();
    static bool isDST(DateTime date);
    static int getSunriseMinutes();
    static int getSunsetMinutes();
    static int getCurrentTimeMinutes();
    static bool updateRTCTime();
    static void updateTimeTask(void *parameter);
    static void calculateSunriseSunsetTask(void *parameter);
    static void formatTime(int timeInMinutes, char *buffer);

    static RTC_DS3231 rtc;
    static NTPClient timeClient;
    static Dusk2Dawn d2d;
    static int sunriseMinutes;
    static int sunsetMinutes;
};