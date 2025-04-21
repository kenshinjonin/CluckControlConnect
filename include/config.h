// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#pragma once

#include <Arduino.h>
#include <cstdint>

// Paramètres WiFi
struct Config {
    const char *ssid;
    const char *password;
    double latitude;
    double longitude;
    int UTC_OFFSET;
    uint8_t buttonPinNumber;
    uint8_t motorEnablePinNumber;
    uint8_t motorInput1PinNumber;
    uint8_t motorInput2PinNumber;
    uint8_t upperLimitSwitchPinNumber;
    uint8_t lowerLimitSwitchPinNumber;
    unsigned long buttonDebounceDelay;
    unsigned long maxDoorMovementDuration;
    int DST_START_MONTH;
    int DST_END_MONTH;
    int DST_CHANGE_DAY;
};

extern Config config;

// Paramètres géographiques
const double latitude = 46.308352907180954;
const double longitude = 4.887889949495334;
int UTC_OFFSET = 1;

// Paramètres des broches
const uint8_t buttonPinNumber = 2;
const uint8_t motorEnablePinNumber = 4;
const uint8_t motorInput1PinNumber = 5;
const uint8_t motorInput2PinNumber = 6;
const uint8_t upperLimitSwitchPinNumber = 7;
const uint8_t lowerLimitSwitchPinNumber = 8;

// Paramètres de temps
const unsigned long buttonDebounceDelay = 500;
const unsigned long maxDoorMovementDuration = 22000;

// Paramètres DST
const int DST_START_MONTH = 3;
const int DST_END_MONTH = 10;
const int DST_CHANGE_DAY = 25;