/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * main.cpp
 * --------
 * Point d'entrée principal du firmware CluckControl Connect.
 *   - Initialise tous les modules (WiFi, temps, porte, serveur web, MQTT)
 *   - Crée les tâches FreeRTOS pour la gestion asynchrone
 *   - La boucle loop() reste vide car tout est géré par les tâches
 */

#include "wifi_manager.h"
#include "time_manager.h"
#include "door_controller.h"
#include "web_server.h"
#include "mqtt_manager.h"

void setup() {
    Serial.begin(115200); // Initialisation du port série pour le debug
    
    // --- Initialisation des modules principaux ---
    WiFiManager::connect();           // Connexion au réseau WiFi
    TimeManager::init();              // Initialisation de la gestion du temps (RTC/NTP)
    DoorController::init();           // Initialisation du contrôleur de porte
    WebServerManager::init();         // Démarrage du serveur web
    MQTTManager::init();              // Initialisation du client MQTT
    
    // --- Création des tâches FreeRTOS pour la gestion asynchrone ---
    xTaskCreatePinnedToCore(WiFiManager::reconnectTask, "WiFiReconnectTask", 4096, NULL, 1, NULL, 0); // Surveillance WiFi
    xTaskCreatePinnedToCore(TimeManager::updateTimeTask, "updateTimeTask", 4096, NULL, 1, NULL, 0); // MAJ RTC/NTP
    xTaskCreatePinnedToCore(TimeManager::calculateSunriseSunsetTask, "calculateSunriseSunsetTask", 4096, NULL, 1, NULL, 0); // Calcul lever/coucher
    xTaskCreatePinnedToCore(DoorController::openDoorTask, "openDoorTask", 4096, NULL, 1, NULL, 0); // Tâche ouverture porte
    xTaskCreatePinnedToCore(DoorController::closeDoorTask, "closeDoorTask", 4096, NULL, 1, NULL, 0); // Tâche fermeture porte
    xTaskCreatePinnedToCore(DoorController::buttonTask, "buttonTask", 4096, NULL, 2, NULL, 0); // Gestion bouton poussoir
    xTaskCreatePinnedToCore(MQTTManager::mqttTask, "mqttTask", 4096, NULL, 1, NULL, 0); // Tâche MQTT
}

void loop() {
    // Vide car tout est géré par les tâches FreeRTOS
}