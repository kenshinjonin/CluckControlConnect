// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include "config.h"

Config config = {
    .ssid = "SSID",
    .password = "PASSWORD",
    .latitude = 46.308352907180954,
    .longitude = 4.887889949495334,
    .UTC_OFFSET = 1,
    .buttonPinNumber = 2,
    .motorEnablePinNumber = 4,
    .motorInput1PinNumber = 5,
    .motorInput2PinNumber = 6,
    .upperLimitSwitchPinNumber = 7,
    .lowerLimitSwitchPinNumber = 8,
    .buttonDebounceDelay = 500,
    .maxDoorMovementDuration = 22000,
    .DST_START_MONTH = 3,
    .DST_END_MONTH = 10,
    .DST_CHANGE_DAY = 25
};
// ...existing code...
