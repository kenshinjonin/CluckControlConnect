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
    .ssid = "SSID",                  // SSID du réseau WiFi (à personnaliser)
    .password = "PASSWORD",          // Mot de passe WiFi (à personnaliser)

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
    .buttonPinNumber = 2,             // Broche du bouton poussoir
    .motorEnablePinNumber = 4,        // Broche Enable du moteur
    .motorInput1PinNumber = 5,        // Broche IN1 du moteur
    .motorInput2PinNumber = 6,        // Broche IN2 du moteur
    .upperLimitSwitchPinNumber = 7,   // Fin de course haut
    .lowerLimitSwitchPinNumber = 8,   // Fin de course bas

    // === Paramètres de temporisation ===
    .buttonDebounceDelay = 500,       // Anti-rebond bouton (ms)
    .maxDoorMovementDuration = 22000, // Durée max mouvement porte (ms)

    // === Paramètres heure d'été (DST) ===
    .dstEnabled = SECRET_DST_ENABLED, // true pour activer l'heure d'été, false sinon
    .DST_START_MONTH = SECRET_DST_START_MONTH, // Mois de début de l'heure d'été
    .DST_END_MONTH = SECRET_DST_END_MONTH,     // Mois de fin de l'heure d'été
};
