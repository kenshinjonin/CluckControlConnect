/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * main.cpp
 * --------
 * Point d'entrée principal du firmware CluckControl Connect.
 *   - Initialise tous les modules (WiFi, temps, porte, serveur web, MQTT)
 *   - Crée les tâches FreeRTOS pour la gestion asynchrone
 *   - Supprime la tâche Arduino par défaut : tout est géré par les tâches
 */

#include <esp_task_wdt.h>
#include "config.h"
#include "wifi_manager.h"
#include "time_manager.h"
#include "door_controller.h"
#include "web_server.h"
#include "mqtt_manager.h"

// Un blocage de plus de 30 s d'une tâche surveillée provoque un redémarrage.
// Les tâches à longue période fractionnent leur attente (voir watchdogDelay)
// afin de rester compatibles avec ce délai court.
static const uint32_t WATCHDOG_TIMEOUT_S = 30;

void setup() {
    Serial.begin(115200); // Initialisation du port série pour le debug
    Serial.printf("\n=== CluckControl Connect %s ===\n", FIRMWARE_VERSION);

    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, true);

    // --- Initialisation des modules principaux ---
    // La porte d'abord : la connexion WiFi peut attendre 10 s, pendant lesquelles
    // les entrées du pont en H flotteraient (GPIO14/IN2 est tiré au niveau haut
    // au reset). Ses publications MQTT sont ignorées tant que MQTT n'existe pas.
    DoorController::init();           // Contrôleur de porte (met le moteur à l'arrêt)
    WiFiManager::connect();           // Connexion au réseau WiFi
    TimeManager::init();              // Gestion du temps (NVS, RTC/NTP, éphémérides)
    MQTTManager::init();              // Client MQTT
    WebServerManager::init();         // Démarrage du serveur web

    // --- Création des tâches FreeRTOS ---
    // Coeur 0 : tâches réseau (cohabitent avec la pile WiFi)
    xTaskCreatePinnedToCore(WiFiManager::reconnectTask, "WiFiReconnectTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TimeManager::updateTimeTask, "updateTimeTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(MQTTManager::mqttTask, "mqttTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(WebServerManager::webServerTask, "webServerTask", 8192, NULL, 1, NULL, 0);
    // Coeur 1 : contrôle temps réel de la porte (insensible aux à-coups de la pile réseau)
    xTaskCreatePinnedToCore(TimeManager::calculateSunriseSunsetTask, "sunriseSunsetTask", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(DoorController::doorScheduleTask, "doorScheduleTask", 4096, NULL, 1, NULL, 1);
    // Priorité supérieure : la boucle de sécurité doit primer sur tout le reste
    xTaskCreatePinnedToCore(DoorController::doorControlTask, "doorControlTask", 4096, NULL, 2, NULL, 1);

    // La tâche Arduino par défaut appelle loop() sans jamais céder le processeur :
    // la conserver avec un loop() vide occuperait un coeur à 100 % pour rien et
    // priverait la tâche inactive du coeur 1 de temps d'exécution.
    vTaskDelete(NULL);
}

void loop() {
    // Jamais atteint : la tâche Arduino est supprimée à la fin de setup()
}
