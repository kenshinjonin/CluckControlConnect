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
#include "config.h"

// Topics d'état publiés par la porte (via publishTopic)
static const char *const topic_door_position = DEVICE_ID "/door/position";
static const char *const topic_limit_switch_upper = DEVICE_ID "/door/limit_switch_upper";
static const char *const topic_limit_switch_lower = DEVICE_ID "/door/limit_switch_lower";
static const char *const topic_button = DEVICE_ID "/button/state";
static const char *const topic_last_event = DEVICE_ID "/door/last_event";
static const char *const topic_alert = DEVICE_ID "/alert";

class MQTTManager {
public:
    static void init();
    static void callback(char *topic, byte *payload, unsigned int length);
    static void mqttTask(void *parameter);
    static void publishHADiscovery();
    static void publishSettings();
    static void publishTopic(const char *topic, const char *value);
    static bool isConnected();

private:
    static void loop();
    static void reconnect();
};

#endif // MQTT_MANAGER_H
