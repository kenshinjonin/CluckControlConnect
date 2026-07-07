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

#include <esp_task_wdt.h>
#include "wifi_manager.h"
#include "time_manager.h"
#include "door_controller.h"
#include "web_server.h"
#include "mqtt_manager.h"

void setup() {
    Serial.begin(115200); // Initialisation du port série pour le debug

    // Watchdog de tâches : reboot si une tâche surveillée se fige plus de 2 minutes
    // (les tâches à longue période NTP/éphémérides ne sont pas surveillées)
    esp_task_wdt_init(120, true);

    // --- Initialisation des modules principaux ---
    WiFiManager::connect();           // Connexion au réseau WiFi
    TimeManager::init();              // Initialisation de la gestion du temps (RTC/NTP)
    DoorController::init();           // Initialisation du contrôleur de porte
    WebServerManager::init();         // Démarrage du serveur web
    MQTTManager::init();              // Initialisation du client MQTT

    // --- Création des tâches FreeRTOS pour la gestion asynchrone ---
    // Coeur 0 : tâches réseau (cohabitent avec la pile WiFi)
    xTaskCreatePinnedToCore(WiFiManager::reconnectTask, "WiFiReconnectTask", 4096, NULL, 1, NULL, 0); // Surveillance WiFi
    xTaskCreatePinnedToCore(TimeManager::updateTimeTask, "updateTimeTask", 4096, NULL, 1, NULL, 0); // MAJ RTC/NTP
    xTaskCreatePinnedToCore(MQTTManager::mqttTask, "mqttTask", 4096, NULL, 1, NULL, 0); // Tâche MQTT
    xTaskCreatePinnedToCore(WebServerManager::webServerTask, "webServerTask", 8192, NULL, 1, NULL, 0); // Serveur web + OTA
    // Coeur 1 : contrôle temps réel de la porte (insensible aux à-coups de la pile réseau)
    xTaskCreatePinnedToCore(TimeManager::calculateSunriseSunsetTask, "calculateSunriseSunsetTask", 4096, NULL, 1, NULL, 1); // Calcul lever/coucher
    xTaskCreatePinnedToCore(DoorController::doorScheduleTask, "doorScheduleTask", 4096, NULL, 1, NULL, 1); // Asservissement jour/nuit de la porte
    xTaskCreatePinnedToCore(DoorController::buttonTask, "buttonTask", 4096, NULL, 2, NULL, 1); // Gestion bouton poussoir + sécurité moteur
}

void loop() {
    // Vide car tout est géré par les tâches FreeRTOS
}
