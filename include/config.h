/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * config.h
 * --------
 * Structure de configuration principale du système :
 *   - Version du firmware
 *   - Paramètres WiFi
 *   - Paramètres MQTT
 *   - Paramètres géographiques
 *   - Brochage matériel
 *   - Paramètres de temporisation
 *
 */

#pragma once

#include <Arduino.h>
#include <cstdint>
#include "secrets.h"

// Version du firmware : publiée au démarrage sur le port série, exposée dans
// Home Assistant (sw_version de l'appareil + capteur de diagnostic) et affichée
// dans l'interface web. À incrémenter à chaque mise à jour OTA.
#define FIRMWARE_VERSION "2.2.0"

// Identifiant de l'appareil : préfixe des topics MQTT et des identifiants Home
// Assistant (lettres minuscules, chiffres et _ uniquement). La valeur par défaut
// conserve les topics et entités existants ; un second poulailler sur le même
// broker se compile avec build_flags = -DDEVICE_ID=\"poulailler2\".
#ifndef DEVICE_ID
#define DEVICE_ID "poulailler"
#endif

// Structure de configuration principale du système
struct Config {
    // Identifiants WiFi
    const char *ssid;                // SSID du réseau WiFi
    const char *password;            // Mot de passe WiFi

    // Paramètres MQTT
    const char *mqttServer;         // Adresse du serveur MQTT
    int mqttPort;                   // Port du serveur MQTT
    const char *mqttUser;           // Nom d'utilisateur MQTT
    const char *mqttPassword;       // Mot de passe MQTT

    // Paramètres géographiques
    double latitude;                 // Latitude du poulailler
    double longitude;                // Longitude du poulailler
    const char *timezone;            // Règle POSIX TZ : fuseau et heure d'été

    // Brochage matériel
    uint8_t buttonPinNumber;         // Broche du bouton poussoir
    uint8_t motorEnablePinNumber;    // Broche Enable du moteur
    uint8_t motorInput1PinNumber;    // Broche IN1 du moteur
    uint8_t motorInput2PinNumber;    // Broche IN2 du moteur
    uint8_t upperLimitSwitchPinNumber; // Fin de course haut
    uint8_t lowerLimitSwitchPinNumber; // Fin de course bas

    // Paramètres de temporisation
    unsigned long buttonDebounceDelay;      // Anti-rebond bouton (ms)
    unsigned long maxDoorMovementDuration;  // Durée max mouvement porte (ms)
    unsigned long motorReverseDeadTime;     // Temps mort imposé avant de réalimenter le moteur (ms)
    unsigned long automationRetryDelay;     // Délai avant nouvelle tentative après un mouvement en échec (ms)
    uint8_t maxMovementFailures;            // Échecs consécutifs avant blocage de l'automatisme

    // Décalages appliqués au lever/coucher du soleil (minutes).
    // Valeurs par défaut au premier démarrage ; ensuite ajustables depuis Home
    // Assistant et conservées en mémoire non volatile (NVS).
    int defaultSunriseOffsetMinutes; // Décalage d'ouverture (négatif = plus tôt)
    int defaultSunsetOffsetMinutes;  // Décalage de fermeture (positif = plus tard)
};

// Déclaration de la variable globale de configuration
extern Config config;
