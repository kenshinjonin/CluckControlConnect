/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.cpp
 * ---------------
 * Gère la synchronisation et la gestion du temps :
 *   - Horloge système de l'ESP32 synchronisée par SNTP, en UTC
 *   - Fuseau horaire et heure d'été appliqués nativement (règle POSIX TZ)
 *   - RTC DS3231 en UTC : remet l'horloge à l'heure au démarrage, réécrite
 *     après chaque synchronisation NTP
 *   - Calcul automatique du lever/coucher du soleil (Dusk2Dawn)
 *   - Décalages utilisateur persistés en mémoire non volatile (NVS)
 *
 * Sans NTP ni RTC valide, les accesseurs renvoient -1 plutôt qu'une valeur
 * arbitraire : l'automatisme se fige au lieu d'agir sur une heure fantaisiste.
 */

#include "config.h"
#include "time_manager.h"
#include "daytime.h"
#include "task_utils.h"
#include <RTClib.h>
#include <Dusk2Dawn.h>
#include <Preferences.h>
#include <esp_sntp.h>
#include <sys/time.h>

// La RTC n'est touchée que par init() (avant les tâches) puis par
// updateTimeTask : un seul utilisateur, pas de verrou nécessaire.
static RTC_DS3231 rtc;
// Fuseau 0 : horaires en minutes UTC, convertis ensuite par la règle TZ
static Dusk2Dawn d2d(config.latitude, config.longitude, 0);
static Preferences preferences;

// Positionné par SNTP à chaque synchronisation, consommé par updateTimeTask
static volatile bool ntpSynced = false;

// Clés NVS (limitées à 15 caractères)
static const char *NVS_NAMESPACE = "coop";
static const char *NVS_KEY_SUNRISE_OFFSET = "sunriseOff";
static const char *NVS_KEY_SUNSET_OFFSET = "sunsetOff";

// 2100-01-01 UTC : au-delà, une date est tenue pour corrompue
static const time_t MAX_VALID_EPOCH = 4102444800;

volatile int TimeManager::rawSunriseMinutes = -1;
volatile int TimeManager::rawSunsetMinutes = -1;
volatile int TimeManager::sunriseOffsetMinutes = 0;
volatile int TimeManager::sunsetOffsetMinutes = 0;
volatile bool TimeManager::rtcAvailable = false;

// --- Bornage d'un décalage utilisateur ---
static int clampOffset(int minutes) {
    if (minutes < TIME_OFFSET_MIN) return TIME_OFFSET_MIN;
    if (minutes > TIME_OFFSET_MAX) return TIME_OFFSET_MAX;
    return minutes;
}

static bool isValidEpoch(time_t t) {
    return t >= MIN_VALID_EPOCH && t < MAX_VALID_EPOCH;
}

// Appelé par la tâche SNTP : on se contente de lever un drapeau, l'écriture
// I2C de la RTC est faite par updateTimeTask.
static void onNtpSync(struct timeval *tv) {
    ntpSynced = true;
}

// --- Initialisation du module temps (NVS, RTC, SNTP, calcul lever/coucher) ---
void TimeManager::init() {
    preferences.begin(NVS_NAMESPACE, false);
    sunriseOffsetMinutes = clampOffset(preferences.getInt(NVS_KEY_SUNRISE_OFFSET, config.defaultSunriseOffsetMinutes));
    sunsetOffsetMinutes = clampOffset(preferences.getInt(NVS_KEY_SUNSET_OFFSET, config.defaultSunsetOffsetMinutes));
    Serial.printf("Decalages lever/coucher : %+d / %+d min\n", (int)sunriseOffsetMinutes, (int)sunsetOffsetMinutes);

    rtcAvailable = rtc.begin();
    if (!rtcAvailable) {
        Serial.println("RTC DS3231 introuvable : heure disponible uniquement via NTP");
    } else if (rtc.lostPower()) {
        Serial.println("RTC a perdu son alimentation : attente de NTP");
    } else {
        // Une RTC muette renvoie des octets à 0xFF : la date obtenue est rejetée
        time_t rtcTime = rtc.now().unixtime();
        if (isValidEpoch(rtcTime)) {
            struct timeval tv = {rtcTime, 0};
            settimeofday(&tv, nullptr);
        } else {
            Serial.println("Date RTC invalide : attente de NTP");
        }
    }

    sntp_set_time_sync_notification_cb(onNtpSync);
    configTzTime(config.timezone, "pool.ntp.org");

    calculateSunriseSunset();
}

// --- Calcul du lever et coucher du soleil, en heure locale ---
void TimeManager::calculateSunriseSunset() {
    time_t now = time(nullptr);
    if (!isValidEpoch(now)) return;

    struct tm utc;
    gmtime_r(&now, &utc);
    int sunriseUtc = d2d.sunrise(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, false);
    int sunsetUtc = d2d.sunset(utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, false);

    // Dusk2Dawn ne signale pas ses échecs : sous une latitude où le soleil ne se
    // lève pas, le calcul interne produit un NaN dont la conversion en int est un
    // comportement indéfini. On rejette donc toute valeur hors de la journée UTC
    // élargie d'un jour de part et d'autre (fuseaux extrêmes).
    if (sunriseUtc <= -1440 || sunriseUtc >= 2880 || sunsetUtc <= -1440 || sunsetUtc >= 2880) {
        Serial.println("Ephemerides hors plage : valeurs precedentes conservees");
        return;
    }

    // Conversion par l'heure absolue de l'événement : le fuseau et l'heure
    // d'été en vigueur à cet instant précis sont appliqués par la règle TZ.
    time_t utcMidnight = now - (utc.tm_hour * 3600 + utc.tm_min * 60 + utc.tm_sec);
    rawSunriseMinutes = localMinutesOf(utcMidnight + sunriseUtc * 60);
    rawSunsetMinutes = localMinutesOf(utcMidnight + sunsetUtc * 60);

    // Affichage des horaires effectifs, décalages utilisateur inclus
    char sunriseTime[6], sunsetTime[6];
    formatTime(getSunriseMinutes(), sunriseTime, sizeof(sunriseTime));
    formatTime(getSunsetMinutes(), sunsetTime, sizeof(sunsetTime));
    Serial.printf("Ouverture: %s | Fermeture: %s\n", sunriseTime, sunsetTime);
}

// --- Horaires effectifs, décalages utilisateur appliqués ---
int TimeManager::getSunriseMinutes() {
    if (rawSunriseMinutes < 0) return -1;
    return ((rawSunriseMinutes + sunriseOffsetMinutes) % 1440 + 1440) % 1440;
}

int TimeManager::getSunsetMinutes() {
    if (rawSunsetMinutes < 0) return -1;
    return ((rawSunsetMinutes + sunsetOffsetMinutes) % 1440 + 1440) % 1440;
}

int TimeManager::getSunriseOffset() {
    return sunriseOffsetMinutes;
}

int TimeManager::getSunsetOffset() {
    return sunsetOffsetMinutes;
}

// Les écritures NVS ne sont faites qu'en cas de changement réel : une
// automatisation Home Assistant qui réémettrait la même valeur en boucle
// userait inutilement la flash.
void TimeManager::setSunriseOffset(int minutes) {
    int value = clampOffset(minutes);
    if (value == sunriseOffsetMinutes) return;
    sunriseOffsetMinutes = value;
    preferences.putInt(NVS_KEY_SUNRISE_OFFSET, value);
    Serial.printf("Nouveau decalage d'ouverture : %+d min\n", value);
}

void TimeManager::setSunsetOffset(int minutes) {
    int value = clampOffset(minutes);
    if (value == sunsetOffsetMinutes) return;
    sunsetOffsetMinutes = value;
    preferences.putInt(NVS_KEY_SUNSET_OFFSET, value);
    Serial.printf("Nouveau decalage de fermeture : %+d min\n", value);
}

// --- Heure locale actuelle en minutes depuis minuit ---
// Lecture de l'horloge système : aucun accès I2C dans la boucle de la porte.
int TimeManager::getCurrentTimeMinutes() {
    time_t now = time(nullptr);
    if (!isValidEpoch(now)) return -1;
    return localMinutesOf(now);
}

// --- Tâche FreeRTOS : recopie de l'heure NTP dans la RTC ---
void TimeManager::updateTimeTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        if (ntpSynced) {
            ntpSynced = false;
            time_t now = time(nullptr);
            // Une réponse NTP fautive écrirait une date absurde dans la RTC
            // sauvegardée par pile : elle est rejetée.
            if (rtcAvailable && isValidEpoch(now)) {
                rtc.adjust(DateTime((uint32_t)now));
            }
        }
        watchdogDelay(60000);
    }
}

// --- Tâche FreeRTOS : calcul lever/coucher du soleil ---
void TimeManager::calculateSunriseSunsetTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        calculateSunriseSunset();
        // Recalcul horaire (changement de date, heure d'été) ; toutes les
        // minutes tant que l'heure n'est pas connue, pour reprendre dès NTP.
        watchdogDelay(rawSunriseMinutes < 0 ? 60000 : 3600000);
    }
}

// --- Formatage d'un horaire (minutes -> HH:MM) ---
void TimeManager::formatTime(int timeInMinutes, char *buffer, size_t size) {
    if (buffer == nullptr || size == 0) return;
    if (timeInMinutes < 0 || timeInMinutes >= 1440) {
        snprintf(buffer, size, "--:--");
        return;
    }
    snprintf(buffer, size, "%02d:%02d", timeInMinutes / 60, timeInMinutes % 60);
}
