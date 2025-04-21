// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include "wifi_manager.h"
#include "time_manager.h"
#include "door_controller.h"
#include "web_server.h"
#include "mqtt_manager.h"

void setup() {
    Serial.begin(115200);
    
    WiFiManager::connect();
    TimeManager::init();
    DoorController::init();
    WebServerManager::init();
    MQTTManager::init();
    
    // Création des tâches
    xTaskCreatePinnedToCore(WiFiManager::reconnectTask, "WiFiReconnectTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TimeManager::updateTimeTask, "updateTimeTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TimeManager::calculateSunriseSunsetTask, "calculateSunriseSunsetTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(DoorController::openDoorTask, "openDoorTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(DoorController::closeDoorTask, "closeDoorTask", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(DoorController::buttonTask, "buttonTask", 4096, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(MQTTManager::mqttTask, "mqttTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
    // Vide car tout est géré par les tâches
}