/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * time_manager.cpp
 * ---------------
 * Gère la synchronisation et la gestion du temps :
 *   - Initialisation RTC/NTP
 *   - Calcul automatique du lever/coucher du soleil (Dusk2Dawn)
 *   - Décalages utilisateur persistés en mémoire non volatile (NVS)
 *   - Prise en compte de l'heure d'été (DST)
 *   - Tâches FreeRTOS pour la mise à jour de l'heure et du lever/coucher
 *
 * Convention horaire : la RTC est réglée en heure d'hiver toute l'année
 * (offset NTP fixe). Le décalage d'été (+60 min) est appliqué de façon
 * cohérente à la fois aux horaires de lever/coucher et à l'heure courante
 * retournée par getCurrentTimeMinutes().
 *
 * En l'absence de RTC ou sur une date invalide, les accesseurs renvoient -1
 * plutôt qu'une valeur arbitraire : l'automatisme se fige au lieu d'agir sur
 * une heure fantaisiste.
 */

#include "config.h"
#include "time_manager.h"
#include "task_utils.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>

WiFiUDP ntpUDP;
RTC_DS3231 TimeManager::rtc;
NTPClient TimeManager::timeClient(ntpUDP, "pool.ntp.org", config.timezoneOffset * 3600); // Décalage UTC en hiver, DST ajouté dynamiquement
Dusk2Dawn TimeManager::d2d(config.latitude, config.longitude, config.timezoneOffset); // Décalage UTC en hiver, DST ajouté dynamiquement

static Preferences preferences;

// Le bus I2C de la RTC est sollicité depuis les deux coeurs : lecture de l'heure
// par l'asservissement de la porte (coeur 1) et écriture NTP horaire (coeur 0).
// Une lecture DS3231 est une transaction en deux temps (positionnement du
// pointeur de registre puis lecture) qu'une écriture concurrente décalerait.
// Aucun autre verrou n'est pris pendant sa détention : pas de risque d'interblocage.
static SemaphoreHandle_t rtcMutex = nullptr;

class RTCLock {
public:
    explicit RTCLock(uint32_t timeoutMs) : acquired(false) {
        if (rtcMutex != nullptr) {
            acquired = xSemaphoreTake(rtcMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE;
        }
    }
    ~RTCLock() {
        if (acquired) xSemaphoreGive(rtcMutex);
    }
    bool ok() const { return acquired; }

private:
    bool acquired;
};

// Clés NVS (limitées à 15 caractères)
static const char *NVS_NAMESPACE = "coop";
static const char *NVS_KEY_SUNRISE_OFFSET = "sunriseOff";
static const char *NVS_KEY_SUNSET_OFFSET = "sunsetOff";

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

// --- Validation d'une date issue de la RTC ---
// Une RTC absente ou muette renvoie des octets à 0xFF : la date obtenue passe
// alors les vérifications naïves mais produit des éphémérides absurdes.
static bool isPlausibleDate(const DateTime &date) {
    return date.isValid() && date.year() >= 2024 && date.year() <= 2099;
}

// --- Initialisation du module temps (NVS, RTC/NTP, calcul lever/coucher) ---
void TimeManager::init() {
    rtcMutex = xSemaphoreCreateMutex();
    preferences.begin(NVS_NAMESPACE, false);
    sunriseOffsetMinutes = clampOffset(preferences.getInt(NVS_KEY_SUNRISE_OFFSET, config.defaultSunriseOffsetMinutes));
    sunsetOffsetMinutes = clampOffset(preferences.getInt(NVS_KEY_SUNSET_OFFSET, config.defaultSunsetOffsetMinutes));
    Serial.printf("Decalages lever/coucher : %+d / %+d min\n", (int)sunriseOffsetMinutes, (int)sunsetOffsetMinutes);

    if (!rtc.begin()) {
        Serial.println("RTC DS3231 introuvable : asservissement horaire desactive");
        rtcAvailable = false;
        return;
    }
    rtcAvailable = true;

    timeClient.begin(); // Doit précéder tout appel à updateRTCTime()

    if (rtc.lostPower()) {
        Serial.println("RTC a perdu son alimentation, initialisation avec l'heure NTP");
    }

    updateRTCTime();
    calculateSunriseSunset();
}

bool TimeManager::isRTCAvailable() {
    return rtcAvailable;
}

// --- Calcul du lever et coucher du soleil (avec ajustement DST) ---
void TimeManager::calculateSunriseSunset() {
    if (!rtcAvailable) return;

    RTCLock lock(1000);
    if (!lock.ok()) return;

    DateTime now = rtc.now();
    if (!isPlausibleDate(now)) {
        Serial.println("Date RTC invalide : ephemerides non recalculees");
        return;
    }

    // Calcul des horaires en minutes depuis minuit (heure d'hiver)
    int sunrise = d2d.sunrise(now.year(), now.month(), now.day(), false);
    int sunset = d2d.sunset(now.year(), now.month(), now.day(), false);

    // Dusk2Dawn ne signale pas ses échecs : sous une latitude où le soleil ne se
    // lève pas, le calcul interne produit un NaN dont la conversion en int est un
    // comportement indéfini. On rejette donc toute valeur hors plage.
    if (sunrise < 0 || sunrise >= 1440 || sunset < 0 || sunset >= 1440) {
        Serial.println("Ephemerides hors plage : valeurs precedentes conservees");
        return;
    }

    // Ajustement pour l'heure d'été (getCurrentTimeMinutes applique le même décalage)
    if (isDST(now)) {
        sunrise += 60;
        sunset += 60;
    }

    rawSunriseMinutes = sunrise % 1440;
    rawSunsetMinutes = sunset % 1440;

    // Affichage des horaires effectifs, décalages utilisateur inclus
    char sunriseTime[6], sunsetTime[6];
    formatTime(getSunriseMinutes(), sunriseTime, sizeof(sunriseTime));
    formatTime(getSunsetMinutes(), sunsetTime, sizeof(sunsetTime));
    Serial.printf("Ouverture: %s | Fermeture: %s\n", sunriseTime, sunsetTime);
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

// --- Obtenir l'heure locale actuelle en minutes depuis minuit ---
// La RTC est en heure d'hiver : on applique le décalage d'été pour rester
// cohérent avec les horaires de lever/coucher ajustés dans calculateSunriseSunset().
int TimeManager::getCurrentTimeMinutes() {
    if (!rtcAvailable) return -1;

    // Bus occupé : on signale l'heure comme indisponible plutôt que d'attendre.
    // L'asservissement se contente de sauter un cycle de 30 secondes.
    RTCLock lock(200);
    if (!lock.ok()) return -1;

    DateTime now = rtc.now();
    if (!isPlausibleDate(now)) return -1;

    int minutes = now.hour() * 60 + now.minute();
    if (isDST(now)) {
        minutes += 60;
    }
    return minutes % 1440;
}

// --- Mise à jour de l'heure RTC depuis NTP ---
bool TimeManager::updateRTCTime() {
    if (!rtcAvailable || WiFi.status() != WL_CONNECTED) return false;
    if (!timeClient.update()) return false;

    unsigned long epochTime = timeClient.getEpochTime();
    // Une réponse tronquée ou un serveur fautif donnerait une date absurde qui
    // serait ensuite écrite dans la RTC sauvegardée par pile : on la rejette.
    if (epochTime < 1704067200UL || epochTime > 4102444800UL) { // 2024-01-01 .. 2100-01-01
        Serial.println("Heure NTP hors plage, RTC non modifiee");
        return false;
    }

    RTCLock lock(1000);
    if (!lock.ok()) return false;

    rtc.adjust(DateTime((uint32_t)epochTime));
    return true;
}

// --- Tâche FreeRTOS : mise à jour périodique de l'heure RTC ---
void TimeManager::updateTimeTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        updateRTCTime();
        watchdogDelay(3600000); // Mise à jour toutes les heures
    }
}

// --- Tâche FreeRTOS : calcul quotidien lever/coucher du soleil ---
void TimeManager::calculateSunriseSunsetTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();
        calculateSunriseSunset();
        watchdogDelay(3600000); // Recalcul toutes les heures (rattrape un changement de date manqué)
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
