/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * config.h
 * --------
 * Structure de configuration principale du système :
 *   - Paramètres WiFi
 *   - Paramètres géographiques
 *   - Brochage matériel
 *   - Paramètres de temporisation
 *   - Paramètres heure d'été (DST)
 */

#pragma once

#include <Arduino.h>
#include <cstdint>
#include "secrets.h"

// Structure de configuration principale du système
struct Config {
    // Identifiants WiFi
    const char *ssid;                // SSID du réseau WiFi
    const char *password;            // Mot de passe WiFi

    // Paramètres géographiques
    double latitude;                 // Latitude du poulailler
    double longitude;                // Longitude du poulailler
    int UTC_OFFSET;                  // Décalage horaire par rapport à UTC (ex: 1 pour UTC+1)

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

    // Paramètres heure d'été (DST)
    int DST_START_MONTH;            // Mois de début de l'heure d'été
    int DST_END_MONTH;              // Mois de fin de l'heure d'été
};

// Déclaration de la variable globale de configuration
extern Config config;
