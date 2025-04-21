// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <PubSubClient.h>

class MQTTManager {
public:
    static void init();
    static void loop();
    static void publishDoorState(const char* state);
    static void publishTopic(const char* logicalTopic, const char* value);
    static void callback(char* topic, byte* payload, unsigned int length);
    static void reconnect();
    static void mqttTask(void *parameter);
};

#endif // MQTT_MANAGER_H