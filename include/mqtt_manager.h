/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * mqtt_manager.h
 * --------------
 * Déclaration de la classe MQTTManager pour la gestion MQTT :
 *   - Initialisation et gestion de la connexion MQTT
 *   - Publication et souscription des topics
 *   - Publication Home Assistant Discovery
 *   - Tâche FreeRTOS pour la gestion asynchrone
 */

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>

class MQTTManager {
public:
    static void init();
    static void callback(char *topic, byte *payload, unsigned int length);
    static void mqttTask(void *parameter);
    static void publishHADiscovery();
    static void publishSettings();
    static void publishTopic(const char *logicalTopic, const char *value);
    static bool isConnected();

private:
    static void loop();
    static void reconnect();
};

#endif // MQTT_MANAGER_H
