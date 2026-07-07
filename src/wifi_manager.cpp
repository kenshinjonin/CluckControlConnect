/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * wifi_manager.cpp
 * ----------------
 * Gère la connexion WiFi du système :
 *   - Connexion initiale au réseau WiFi
 *   - Affichage du RSSI (force du signal)
 *   - Tâche FreeRTOS pour la reconnexion automatique en cas de perte
 */

#include "wifi_manager.h"
#include "config.h"
#include <esp_task_wdt.h>

WiFiClient WiFiManager::wifiClient;

// --- Connexion initiale au réseau WiFi ---
bool WiFiManager::connect() {
    WiFi.begin(config.ssid, config.password);
    Serial.print("Connexion au WiFi");
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnecté au WiFi");
        Serial.print("Adresse IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println("\nÉchec de connexion au WiFi");
    return false;
}

// --- Affichage du RSSI WiFi sur le port série ---
void WiFiManager::printRSSI() {
    Serial.print("Force du signal (RSSI): ");
    Serial.println(WiFi.RSSI());
}

// --- Tâche FreeRTOS : reconnexion automatique au WiFi ---
void WiFiManager::reconnectTask(void *pvParameters) {
    esp_task_wdt_add(NULL); // Tâche surveillée par le watchdog
    for(;;) {
        esp_task_wdt_reset();
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Connexion WiFi perdue. Tentative de reconnexion...");
            if (!connect()) {
                Serial.println("Échec de reconnexion. Nouvelle tentative dans 30 secondes.");
            }
        }
        // Attendre 30 secondes avant la prochaine vérification
        delay(30000);
    }
}