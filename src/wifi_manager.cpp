// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include "wifi_manager.h"
#include "config.h"

WiFiClient WiFiManager::wifiClient;

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

void WiFiManager::printRSSI() {
    Serial.print("Force du signal (RSSI): ");
    Serial.println(WiFi.RSSI());
}

void WiFiManager::reconnectTask(void *pvParameters) {
    for(;;) {
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