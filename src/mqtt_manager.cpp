// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "secrets.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* mqtt_server = SECRET_MQTT_SERVER;
const char* mqtt_user = SECRET_MQTT_USER;
const char* mqtt_pass = SECRET_MQTT_PASS;
const char* topic_state = "poulailler/door/state";
const char* topic_cmd = "poulailler/door/cmd";
const char* topic_status = "poulailler/status";
const char* topic_log = "poulailler/log";
const char* topic_time = "poulailler/time";
const char* topic_wifi = "poulailler/wifi/rssi";
const char* topic_door_position = "poulailler/door/position";
const char* topic_limit_switch_upper = "poulailler/door/limit_switch_upper";
const char* topic_limit_switch_lower = "poulailler/door/limit_switch_lower";
const char* topic_button = "poulailler/button/state";
const char* topic_uptime = "poulailler/uptime";
const char* topic_last_event = "poulailler/door/last_event";
const char* topic_alert = "poulailler/alert";

class MQTTManager {
public:
    static void init();
    static void loop();
    static void publishDoorState(const char* state);
    static void callback(char* topic, byte* payload, unsigned int length);
    static void reconnect();
    static void mqttTask(void *parameter);
    static void publishHADiscovery();
    static void publishTopic(const char* logicalTopic, const char* value);
};

void MQTTManager::mqttTask(void *parameter) {
    for (;;) {
        MQTTManager::loop();
        // Publier périodiquement l'état de la porte, RSSI, etc.
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 60000) { // toutes les 60 secondes
            // Publier l'état de la porte (exemple, à adapter selon l'état réel)
            // MQTTManager::publishDoorState("OPEN"/"CLOSED");
            // Publier le RSSI WiFi
            char rssiStr[8];
            snprintf(rssiStr, sizeof(rssiStr), "%d", WiFi.RSSI());
            mqttClient.publish(topic_wifi, rssiStr, true);
            lastPublish = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void MQTTManager::reconnect() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect("PoulaillerClient", mqtt_user, mqtt_pass)) {
            mqttClient.subscribe(topic_cmd);
            MQTTManager::publishHADiscovery(); // Publie la config Home Assistant à chaque reconnexion
        } else {
            delay(5000);
        }
    }
}

void MQTTManager::publishHADiscovery() {
    // Device info commun à toutes les entités
    String device = "\"identifiers\":[\"poulailler_esp32_001\"],\"name\":\"Poulailler ESP32\",\"manufacturer\":\"DIY\",\"model\":\"ESP32 Poulailler\",\"sw_version\":\"1.0.0\"";

    // Porte (cover)
    String configDoor = "{"
        "\"name\":\"Porte Poulailler\"," 
        "\"state_topic\":\"poulailler/door/state\"," 
        "\"command_topic\":\"poulailler/door/cmd\"," 
        "\"payload_open\":\"OPEN\"," 
        "\"payload_close\":\"CLOSE\"," 
        "\"payload_stop\":\"STOP\"," 
        "\"state_open\":\"OPEN\"," 
        "\"state_closed\":\"CLOSED\"," 
        "\"device_class\":\"garage\"," 
        "\"unique_id\":\"poulailler_door_001\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/cover/poulailler_door/config", configDoor.c_str(), true);

    // RSSI WiFi
    String configRSSI = "{"
        "\"name\":\"Poulailler WiFi RSSI\"," 
        "\"state_topic\":\"poulailler/wifi/rssi\"," 
        "\"unit_of_measurement\":\"dBm\"," 
        "\"device_class\":\"signal_strength\"," 
        "\"unique_id\":\"poulailler_wifi_rssi_001\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_wifi_rssi/config", configRSSI.c_str(), true);

    // Position de la porte
    String configPosition = "{"
        "\"name\":\"Position Porte Poulailler\"," 
        "\"state_topic\":\"poulailler/door/position\"," 
        "\"unique_id\":\"poulailler_door_position_001\"," 
        "\"device_class\":\"enum\"," 
        "\"icon\":\"mdi:door\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_door_position/config", configPosition.c_str(), true);

    // Fins de course
    String configLimitUpper = "{"
        "\"name\":\"Fin de course haut\"," 
        "\"state_topic\":\"poulailler/door/limit_switch_upper\"," 
        "\"unique_id\":\"poulailler_limit_switch_upper_001\"," 
        "\"device_class\":\"safety\"," 
        "\"icon\":\"mdi:arrow-up-bold-box\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_limit_switch_upper/config", configLimitUpper.c_str(), true);

    String configLimitLower = "{"
        "\"name\":\"Fin de course bas\"," 
        "\"state_topic\":\"poulailler/door/limit_switch_lower\"," 
        "\"unique_id\":\"poulailler_limit_switch_lower_001\"," 
        "\"device_class\":\"safety\"," 
        "\"icon\":\"mdi:arrow-down-bold-box\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_limit_switch_lower/config", configLimitLower.c_str(), true);

    // Bouton poussoir
    String configButton = "{"
        "\"name\":\"Bouton Porte Poulailler\"," 
        "\"state_topic\":\"poulailler/button/state\"," 
        "\"unique_id\":\"poulailler_button_001\"," 
        "\"device_class\":\"occupancy\"," 
        "\"icon\":\"mdi:gesture-tap-button\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_button/config", configButton.c_str(), true);

    // Uptime
    String configUptime = "{"
        "\"name\":\"Poulailler Uptime\"," 
        "\"state_topic\":\"poulailler/uptime\"," 
        "\"unit_of_measurement\":\"s\"," 
        "\"icon\":\"mdi:timer-outline\"," 
        "\"unique_id\":\"poulailler_uptime_001\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_uptime/config", configUptime.c_str(), true);

    // Dernier événement porte
    String configLastEvent = "{"
        "\"name\":\"Dernier événement porte\"," 
        "\"state_topic\":\"poulailler/door/last_event\"," 
        "\"icon\":\"mdi:history\"," 
        "\"unique_id\":\"poulailler_last_event_001\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_last_event/config", configLastEvent.c_str(), true);

    // Alertes
    String configAlert = "{"
        "\"name\":\"Alerte Poulailler\"," 
        "\"state_topic\":\"poulailler/alert\"," 
        "\"icon\":\"mdi:alert\"," 
        "\"unique_id\":\"poulailler_alert_001\"," 
        "\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_alert/config", configAlert.c_str(), true);
}

void MQTTManager::publishTopic(const char* logicalTopic, const char* value) {
    const char* topic = nullptr;
    if (strcmp(logicalTopic, "topic_limit_switch_upper") == 0) topic = topic_limit_switch_upper;
    else if (strcmp(logicalTopic, "topic_limit_switch_lower") == 0) topic = topic_limit_switch_lower;
    else if (strcmp(logicalTopic, "topic_door_position") == 0) topic = topic_door_position;
    else if (strcmp(logicalTopic, "topic_last_event") == 0) topic = topic_last_event;
    else if (strcmp(logicalTopic, "topic_button") == 0) topic = topic_button;
    else topic = logicalTopic; // fallback: publish on the given string
    if (topic) mqttClient.publish(topic, value, true);
}

#endif // MQTT_MANAGER_H