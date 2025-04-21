/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * wifi_manager.h
 * --------------
 * Déclaration de la classe WiFiManager pour la gestion du WiFi :
 *   - Connexion initiale
 *   - Affichage RSSI
 *   - Tâche FreeRTOS de reconnexion
 */

#pragma once

#include <WiFi.h>

class WiFiManager {
public:
    static bool connect();
    static void printRSSI();
    static void reconnectTask(void *pvParameters);
    static WiFiClient wifiClient;
};