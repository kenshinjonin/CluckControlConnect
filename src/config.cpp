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
    // Règle POSIX TZ : UTC+1 l'hiver, UTC+2 du dernier dimanche de mars (2 h)
    // au dernier dimanche d'octobre (3 h). Autres pays : chercher « POSIX TZ »
    // de la ville, par ex. "EST5EDT,M3.2.0,M11.1.0" pour New York.
    .timezone = "CET-1CEST,M3.5.0,M10.5.0/3",

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
};
