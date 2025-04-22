#include "mqtt_manager.h"
#include "config.h"
#include "door_controller.h"

// Clients MQTT et WiFi
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// Topics MQTT
static const char* topic_state = "poulailler/door/state";
static const char* topic_cmd = "poulailler/door/cmd";
static const char* topic_status = "poulailler/status";
static const char* topic_log = "poulailler/log";
static const char* topic_wifi = "poulailler/wifi/rssi";
static const char* topic_door_position = "poulailler/door/position";
static const char* topic_limit_switch_upper = "poulailler/door/limit_switch_upper";
static const char* topic_limit_switch_lower = "poulailler/door/limit_switch_lower";
static const char* topic_button = "poulailler/button/state";
static const char* topic_uptime = "poulailler/uptime";
static const char* topic_last_event = "poulailler/door/last_event";
static const char* topic_alert = "poulailler/alert";

void MQTTManager::init() {
    mqttClient.setServer(config.mqttServer, config.mqttPort);
    mqttClient.setCallback(callback);
    reconnect();
}

void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}

void MQTTManager::publishDoorState(const char* state) {
    if (mqttClient.connected()) {
        mqttClient.publish(topic_state, state, true);
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, topic_cmd) == 0) {
        char command[length + 1];
        memcpy(command, payload, length);
        command[length] = '\0';

        if (strcmp(command, "OPEN") == 0) {
            DoorController::open();
        }
        else if (strcmp(command, "CLOSE") == 0) {
            DoorController::close();
        }
        else if (strcmp(command, "STOP") == 0) {
            DoorController::stop();
        }
    }
}

void MQTTManager::reconnect() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect("PoulaillerClient", config.mqttUser, config.mqttPassword)) {
            mqttClient.subscribe(topic_cmd);
            publishHADiscovery();
        } else {
            delay(5000);
        }
    }
}

void MQTTManager::mqttTask(void *parameter) {
    for (;;) {
        loop();
        static unsigned long lastPublish = 0;
        if (millis() - lastPublish > 60000) {
            char rssiStr[8];
            snprintf(rssiStr, sizeof(rssiStr), "%d", WiFi.RSSI());
            mqttClient.publish(topic_wifi, rssiStr, true);
            lastPublish = millis();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void MQTTManager::publishHADiscovery() {
    String device = "\"identifiers\":[\"poulailler_esp32_001\"],\"name\":\"Poulailler ESP32\",\"manufacturer\":\"DIY\",\"model\":\"ESP32 Poulailler\",\"sw_version\":\"1.0.0\"";

    String configDoor = "{\"name\":\"Porte Poulailler\",\"state_topic\":\"poulailler/door/state\",\"command_topic\":\"poulailler/door/cmd\",\"payload_open\":\"OPEN\",\"payload_close\":\"CLOSE\",\"payload_stop\":\"STOP\",\"state_open\":\"OPEN\",\"state_closed\":\"CLOSED\",\"device_class\":\"garage\",\"unique_id\":\"poulailler_door_001\",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/cover/poulailler_door/config", configDoor.c_str(), true);

    String configRSSI = "{\"name\":\"Poulailler WiFi RSSI\",\"state_topic\":\"poulailler/wifi/rssi\",\"unit_of_measurement\":\"dBm\",\"device_class\":\"signal_strength\",\"unique_id\":\"poulailler_wifi_rssi_001\",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_wifi_rssi/config", configRSSI.c_str(), true);
}

void MQTTManager::publishTopic(const char* logicalTopic, const char* value) {
    const char* topic = nullptr;
    if (strcmp(logicalTopic, "topic_limit_switch_upper") == 0) topic = topic_limit_switch_upper;
    else if (strcmp(logicalTopic, "topic_limit_switch_lower") == 0) topic = topic_limit_switch_lower;
    else if (strcmp(logicalTopic, "topic_door_position") == 0) topic = topic_door_position;
    else if (strcmp(logicalTopic, "topic_last_event") == 0) topic = topic_last_event;
    else if (strcmp(logicalTopic, "topic_button") == 0) topic = topic_button;
    else topic = logicalTopic;
    
    if (topic && mqttClient.connected()) {
        mqttClient.publish(topic, value, true);
    }
}