/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * mqtt_manager.cpp
 * ----------------
 * Gère la connexion MQTT et l'intégration Home Assistant :
 *   - Connexion non bloquante avec nouvelle tentative périodique
 *   - Last Will Testament (poulailler/status online/offline)
 *   - Publication des états et souscription aux commandes
 *   - Home Assistant Discovery avec disponibilité
 *   - Accès PubSubClient protégé par mutex (publications multi-tâches)
 */

#include "mqtt_manager.h"
#include "config.h"
#include "door_controller.h"
#include <esp_task_wdt.h>

// Clients MQTT et WiFi
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// Mutex récursif : PubSubClient n'est pas thread-safe et les publications
// viennent de plusieurs tâches (bouton, planificateur, web, MQTT).
// Récursif car le callback (exécuté sous mutex) peut lui-même republier.
static SemaphoreHandle_t mqttMutex = nullptr;

// Identifiant client unique (suffixe dérivé de l'adresse MAC)
static char clientId[32] = "PoulaillerClient";

// Topics MQTT
static const char* topic_state = "poulailler/door/state";
static const char* topic_cmd = "poulailler/door/cmd";
static const char* topic_status = "poulailler/status";
static const char* topic_wifi = "poulailler/wifi/rssi";
static const char* topic_door_position = "poulailler/door/position";
static const char* topic_limit_switch_upper = "poulailler/door/limit_switch_upper";
static const char* topic_limit_switch_lower = "poulailler/door/limit_switch_lower";
static const char* topic_button = "poulailler/button/state";
static const char* topic_uptime = "poulailler/uptime";
static const char* topic_last_event = "poulailler/door/last_event";
static const char* topic_alert = "poulailler/alert";

void MQTTManager::init() {
    mqttMutex = xSemaphoreCreateRecursiveMutex();
    uint64_t mac = ESP.getEfuseMac();
    snprintf(clientId, sizeof(clientId), "poulailler-%04X%08X", (uint16_t)(mac >> 32), (uint32_t)mac);
    mqttClient.setServer(config.mqttServer, config.mqttPort);
    mqttClient.setCallback(callback);
    // Pas de connexion ici : mqttTask s'en charge de façon non bloquante,
    // le système reste fonctionnel même si le broker est injoignable au boot
}

void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}

void MQTTManager::publishDoorState(const char* state) {
    if (mqttMutex == nullptr) return;
    if (xSemaphoreTakeRecursive(mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic_state, state, true);
        }
        xSemaphoreGiveRecursive(mqttMutex);
    }
}

void MQTTManager::callback(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, topic_cmd) == 0) {
        char command[length + 1];
        memcpy(command, payload, length);
        command[length] = '\0';

        DoorController::setManualOverride();
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

// Tentative de connexion unique et non bloquante, au plus une fois toutes les 5 secondes.
// Appelée depuis loop() sous mutex (contexte mqttTask).
void MQTTManager::reconnect() {
    static unsigned long lastAttemptTimestamp = 0;
    if (lastAttemptTimestamp != 0 && millis() - lastAttemptTimestamp < 5000) return;
    lastAttemptTimestamp = millis();

    if (WiFi.status() != WL_CONNECTED) return;

    // LWT : le broker publie "offline" sur poulailler/status si l'ESP32 disparaît
    if (mqttClient.connect(clientId, config.mqttUser, config.mqttPassword,
                           topic_status, 0, true, "offline")) {
        mqttClient.publish(topic_status, "online", true);
        mqttClient.subscribe(topic_cmd);
        publishHADiscovery();
    }
}

void MQTTManager::mqttTask(void *parameter) {
    esp_task_wdt_add(NULL); // Tâche surveillée par le watchdog
    for (;;) {
        esp_task_wdt_reset();
        if (mqttMutex != nullptr && xSemaphoreTakeRecursive(mqttMutex, portMAX_DELAY) == pdTRUE) {
            loop();
            static unsigned long lastPublish = 0;
            if (mqttClient.connected() && millis() - lastPublish > 60000) {
                char buffer[16];
                snprintf(buffer, sizeof(buffer), "%d", WiFi.RSSI());
                mqttClient.publish(topic_wifi, buffer, true);
                snprintf(buffer, sizeof(buffer), "%lu", millis() / 1000);
                mqttClient.publish(topic_uptime, buffer, true);
                lastPublish = millis();
            }
            xSemaphoreGiveRecursive(mqttMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// Publication des configurations Home Assistant Discovery.
// Appelée depuis reconnect(), donc déjà sous mutex.
void MQTTManager::publishHADiscovery() {
    String device = "\"identifiers\":[\"poulailler_esp32_001\"],\"name\":\"Poulailler ESP32\",\"manufacturer\":\"DIY\",\"model\":\"ESP32 Poulailler\",\"sw_version\":\"1.0.0\"";
    String availability = "\"availability_topic\":\"poulailler/status\"";

    // Le cover s'appuie sur poulailler/door/position (état réel des fins de course),
    // pas sur poulailler/door/state qui n'est qu'un écho des commandes
    String configDoor = "{\"name\":\"Porte Poulailler\",\"state_topic\":\"poulailler/door/position\",\"command_topic\":\"poulailler/door/cmd\",\"payload_open\":\"OPEN\",\"payload_close\":\"CLOSE\",\"payload_stop\":\"STOP\",\"state_open\":\"OPEN\",\"state_closed\":\"CLOSED\",\"state_opening\":\"OPENING\",\"state_closing\":\"CLOSING\",\"device_class\":\"garage\",\"unique_id\":\"poulailler_door_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/cover/poulailler_door/config", configDoor.c_str(), true);

    String configRSSI = "{\"name\":\"Poulailler WiFi RSSI\",\"state_topic\":\"poulailler/wifi/rssi\",\"unit_of_measurement\":\"dBm\",\"device_class\":\"signal_strength\",\"unique_id\":\"poulailler_wifi_rssi_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_wifi_rssi/config", configRSSI.c_str(), true);

    String configUptime = "{\"name\":\"Poulailler Uptime\",\"state_topic\":\"poulailler/uptime\",\"unit_of_measurement\":\"s\",\"unique_id\":\"poulailler_uptime_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_uptime/config", configUptime.c_str(), true);

    String configPosition = "{\"name\":\"Poulailler Position Porte\",\"state_topic\":\"poulailler/door/position\",\"unique_id\":\"poulailler_door_position_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_door_position/config", configPosition.c_str(), true);

    String configLimitUpper = "{\"name\":\"Poulailler Fin de course haut\",\"state_topic\":\"poulailler/door/limit_switch_upper\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"door\",\"unique_id\":\"poulailler_limit_upper_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_limit_upper/config", configLimitUpper.c_str(), true);

    String configLimitLower = "{\"name\":\"Poulailler Fin de course bas\",\"state_topic\":\"poulailler/door/limit_switch_lower\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"door\",\"unique_id\":\"poulailler_limit_lower_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_limit_lower/config", configLimitLower.c_str(), true);

    String configButton = "{\"name\":\"Poulailler Bouton poussoir\",\"state_topic\":\"poulailler/button/state\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"unique_id\":\"poulailler_button_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/binary_sensor/poulailler_button/config", configButton.c_str(), true);

    String configLastEvent = "{\"name\":\"Poulailler Dernier événement\",\"state_topic\":\"poulailler/door/last_event\",\"unique_id\":\"poulailler_last_event_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_last_event/config", configLastEvent.c_str(), true);

    String configAlert = "{\"name\":\"Poulailler Alerte\",\"state_topic\":\"poulailler/alert\",\"unique_id\":\"poulailler_alert_001\"," + availability + ",\"device\":{" + device + "}}";
    mqttClient.publish("homeassistant/sensor/poulailler_alert/config", configAlert.c_str(), true);
}

void MQTTManager::publishTopic(const char* logicalTopic, const char* value) {
    const char* topic = nullptr;
    if (strcmp(logicalTopic, "topic_limit_switch_upper") == 0) topic = topic_limit_switch_upper;
    else if (strcmp(logicalTopic, "topic_limit_switch_lower") == 0) topic = topic_limit_switch_lower;
    else if (strcmp(logicalTopic, "topic_door_position") == 0) topic = topic_door_position;
    else if (strcmp(logicalTopic, "topic_last_event") == 0) topic = topic_last_event;
    else if (strcmp(logicalTopic, "topic_button") == 0) topic = topic_button;
    else if (strcmp(logicalTopic, "topic_alert") == 0) topic = topic_alert;
    else topic = logicalTopic;

    if (topic == nullptr || mqttMutex == nullptr) return;
    if (xSemaphoreTakeRecursive(mqttMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, value, true);
        }
        xSemaphoreGiveRecursive(mqttMutex);
    }
}
