/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * config.cpp
 * -------------
 * Initialise la structure globale de configuration du système (config).
 *   - Centralise toutes les valeurs de configuration matérielle, réseau, géographique et horaire.
 *   - Facilite la maintenance et l'adaptation à un autre matériel ou site.
 */

#include "config.h"

Config config = {
    // === Identifiants WiFi ===
    .ssid = SECRET_SSID,              // SSID du réseau WiFi (depuis secrets.h)
    .password = SECRET_PASSWORD,      // Mot de passe WiFi (depuis secrets.h)

    // === Paramètres MQTT ===
    .mqttServer = SECRET_MQTT_SERVER,
    .mqttPort = 1883,               // Port MQTT par défaut
    .mqttUser = SECRET_MQTT_USER,
    .mqttPassword = SECRET_MQTT_PASS,

    // === Paramètres géographiques ===
    .latitude = SECRET_LATITUDE,      // Latitude du poulailler (depuis secrets.h)
    .longitude = SECRET_LONGITUDE,    // Longitude du poulailler (depuis secrets.h)
    .timezoneOffset = SECRET_TIMEZONE_OFFSET, // Fuseau horaire explicite en hiver (ex: 1 pour UTC+1, la logique DST ajoutera +1 l'été)

    // === Brochage matériel ===
    // GPIO 6-11 interdits (flash SPI interne) ; 0/2/5/12/15 sont des broches de strapping
    .buttonPinNumber = 25,            // Broche du bouton poussoir (pull-up interne)
    .motorEnablePinNumber = 26,       // Broche Enable du moteur
    .motorInput1PinNumber = 27,       // Broche IN1 du moteur
    .motorInput2PinNumber = 14,       // Broche IN2 du moteur
    .upperLimitSwitchPinNumber = 32,  // Fin de course haut (pull-up interne)
    .lowerLimitSwitchPinNumber = 33,  // Fin de course bas (pull-up interne)

    // === Paramètres de temporisation ===
    .buttonDebounceDelay = 500,       // Anti-rebond bouton (ms)
    .maxDoorMovementDuration = 22000, // Durée max mouvement porte (ms)
    .motorReverseDeadTime = 400,      // Roue libre imposée avant réalimentation (ms)
    .automationRetryDelay = 60000,    // Attente avant nouvelle tentative après échec (ms)
    .maxMovementFailures = 3,         // Échecs consécutifs avant blocage (alerte DOOR_JAMMED)

    // === Décalages lever/coucher (minutes, ajustables ensuite depuis Home Assistant) ===
    .defaultSunriseOffsetMinutes = 0,  // Ouverture à l'heure du lever
    .defaultSunsetOffsetMinutes = 20,  // Fermeture 20 min après le coucher : les poules
                                       // rentrent à la tombée du jour, pas au coucher astronomique

    // === Paramètres heure d'été (DST) ===
    .dstEnabled = SECRET_DST_ENABLED, // true pour activer l'heure d'été, false sinon
    .DST_START_MONTH = SECRET_DST_START_MONTH, // Mois de début de l'heure d'été
    .DST_END_MONTH = SECRET_DST_END_MONTH,     // Mois de fin de l'heure d'été
};
