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
 *
 * Les commandes reçues ne pilotent jamais le moteur directement : elles sont
 * déposées dans la file de DoorController. Cela évite une inversion d'ordre de
 * verrouillage (le callback s'exécute avec le mutex MQTT pris, tandis que le
 * pilotage moteur publie et prendrait donc ce même mutex).
 */

#include "mqtt_manager.h"
#include "config.h"
#include "door_controller.h"
#include "time_manager.h"
#include "task_utils.h"

// Clients MQTT et WiFi
static WiFiClient espClient;
static PubSubClient mqttClient(espClient);

// Mutex récursif : PubSubClient n'est pas thread-safe et les publications
// viennent de plusieurs tâches (porte, web, MQTT).
// Récursif car le callback (exécuté sous mutex) peut lui-même republier.
static SemaphoreHandle_t mqttMutex = nullptr;

// Identifiant client unique (suffixe dérivé de l'adresse MAC)
static char clientId[32] = "PoulaillerClient";

// Taille de tampon MQTT. Les messages de Home Assistant Discovery dépassent
// largement les 256 octets par défaut de PubSubClient, qui refuserait alors de
// les émettre sans le signaler : aucune entité n'apparaîtrait dans HA.
static const uint16_t MQTT_BUFFER_SIZE = 1024;

// Topics MQTT
static const char *topic_cmd = "poulailler/door/cmd";
static const char *topic_status = "poulailler/status";
static const char *topic_wifi = "poulailler/wifi/rssi";
static const char *topic_door_position = "poulailler/door/position";
static const char *topic_limit_switch_upper = "poulailler/door/limit_switch_upper";
static const char *topic_limit_switch_lower = "poulailler/door/limit_switch_lower";
static const char *topic_button = "poulailler/button/state";
static const char *topic_uptime = "poulailler/uptime";
static const char *topic_last_event = "poulailler/door/last_event";
static const char *topic_alert = "poulailler/alert";
static const char *topic_firmware = "poulailler/firmware";
static const char *topic_sunrise = "poulailler/sunrise";
static const char *topic_sunset = "poulailler/sunset";
static const char *topic_sunrise_offset = "poulailler/sunrise_offset";
static const char *topic_sunset_offset = "poulailler/sunset_offset";
static const char *topic_sunrise_offset_set = "poulailler/sunrise_offset/set";
static const char *topic_sunset_offset_set = "poulailler/sunset_offset/set";

void MQTTManager::init() {
    mqttMutex = xSemaphoreCreateRecursiveMutex();
    uint64_t mac = ESP.getEfuseMac();
    snprintf(clientId, sizeof(clientId), "poulailler-%04X%08X", (uint16_t)(mac >> 32), (uint32_t)mac);
    mqttClient.setServer(config.mqttServer, config.mqttPort);
    mqttClient.setCallback(callback);
    mqttClient.setBufferSize(MQTT_BUFFER_SIZE);
    // Délai socket court : la tentative de connexion se fait mutex pris, un
    // broker injoignable ne doit donc pas bloquer les publications 15 secondes.
    mqttClient.setSocketTimeout(2);
    // Pas de connexion ici : mqttTask s'en charge de façon non bloquante,
    // le système reste fonctionnel même si le broker est injoignable au boot
}

bool MQTTManager::isConnected() {
    return mqttClient.connected();
}

void MQTTManager::loop() {
    if (!mqttClient.connected()) {
        reconnect();
    }
    mqttClient.loop();
}

// --- Réception des commandes ---
void MQTTManager::callback(char *topic, byte *payload, unsigned int length) {
    // Le broker contrôle `length` : la copie est bornée par la taille du tampon
    // local, jamais par la longueur annoncée.
    char value[32];
    size_t copied = length < sizeof(value) - 1 ? length : sizeof(value) - 1;
    memcpy(value, payload, copied);
    value[copied] = '\0';

    if (strcmp(topic, topic_cmd) == 0) {
        if (strcmp(value, "OPEN") == 0) {
            DoorController::requestCommand(DoorCommand::Open, true);
        } else if (strcmp(value, "CLOSE") == 0) {
            DoorController::requestCommand(DoorCommand::Close, true);
        } else if (strcmp(value, "STOP") == 0) {
            DoorController::requestCommand(DoorCommand::Stop, true);
        }
    } else if (strcmp(topic, topic_sunrise_offset_set) == 0) {
        TimeManager::setSunriseOffset(atoi(value));
        publishSettings();
    } else if (strcmp(topic, topic_sunset_offset_set) == 0) {
        TimeManager::setSunsetOffset(atoi(value));
        publishSettings();
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
        Serial.println("Connecte au broker MQTT");
        mqttClient.publish(topic_status, "online", true);
        mqttClient.subscribe(topic_cmd);
        mqttClient.subscribe(topic_sunrise_offset_set);
        mqttClient.subscribe(topic_sunset_offset_set);
        publishHADiscovery();
        publishSettings();
        // Les publications émises pendant la coupure ont été perdues :
        // on demande une republication complète des états de la porte.
        DoorController::forceStatePublish();
    }
}

void MQTTManager::mqttTask(void *parameter) {
    esp_task_wdt_add(NULL);
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
                publishSettings();
                lastPublish = millis();
            }
            xSemaphoreGiveRecursive(mqttMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// --- Publication d'un message de configuration Home Assistant Discovery ---
// L'échec de publication est signalé : PubSubClient abandonne silencieusement
// tout paquet plus grand que son tampon, ce qui rendrait l'intégration
// inopérante sans le moindre message d'erreur.
static void publishDiscovery(const char *component, const char *objectId, const String &payload) {
    String topic = String("homeassistant/") + component + "/" + objectId + "/config";
    if (!mqttClient.publish(topic.c_str(), payload.c_str(), true)) {
        Serial.printf("Discovery %s non publie (%u octets, tampon %u)\n",
                      objectId, (unsigned)payload.length(), (unsigned)MQTT_BUFFER_SIZE);
    }
}

// Publication des configurations Home Assistant Discovery.
// Appelée depuis reconnect(), donc déjà sous mutex.
void MQTTManager::publishHADiscovery() {
    String device = String("\"identifiers\":[\"poulailler_esp32_001\"],\"name\":\"Poulailler ESP32\","
                           "\"manufacturer\":\"DIY\",\"model\":\"ESP32 Poulailler\",\"sw_version\":\"") +
                    FIRMWARE_VERSION + "\"";
    String suffix = String(",\"availability_topic\":\"") + topic_status + "\",\"device\":{" + device + "}}";

    // Le cover s'appuie sur poulailler/door/position (état réel des fins de course)
    publishDiscovery("cover", "poulailler_door",
        String("{\"name\":\"Porte Poulailler\",\"state_topic\":\"") + topic_door_position +
        "\",\"command_topic\":\"" + topic_cmd +
        "\",\"payload_open\":\"OPEN\",\"payload_close\":\"CLOSE\",\"payload_stop\":\"STOP\","
        "\"state_open\":\"OPEN\",\"state_closed\":\"CLOSED\",\"state_opening\":\"OPENING\","
        "\"state_closing\":\"CLOSING\",\"device_class\":\"garage\","
        "\"unique_id\":\"poulailler_door_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_wifi_rssi",
        String("{\"name\":\"Poulailler WiFi RSSI\",\"state_topic\":\"") + topic_wifi +
        "\",\"unit_of_measurement\":\"dBm\",\"device_class\":\"signal_strength\","
        "\"entity_category\":\"diagnostic\",\"unique_id\":\"poulailler_wifi_rssi_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_uptime",
        String("{\"name\":\"Poulailler Uptime\",\"state_topic\":\"") + topic_uptime +
        "\",\"unit_of_measurement\":\"s\",\"entity_category\":\"diagnostic\","
        "\"unique_id\":\"poulailler_uptime_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_firmware",
        String("{\"name\":\"Poulailler Firmware\",\"state_topic\":\"") + topic_firmware +
        "\",\"entity_category\":\"diagnostic\",\"unique_id\":\"poulailler_firmware_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_door_position",
        String("{\"name\":\"Poulailler Position Porte\",\"state_topic\":\"") + topic_door_position +
        "\",\"unique_id\":\"poulailler_door_position_001\"" + suffix);

    publishDiscovery("binary_sensor", "poulailler_limit_upper",
        String("{\"name\":\"Poulailler Fin de course haut\",\"state_topic\":\"") + topic_limit_switch_upper +
        "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"door\","
        "\"unique_id\":\"poulailler_limit_upper_001\"" + suffix);

    publishDiscovery("binary_sensor", "poulailler_limit_lower",
        String("{\"name\":\"Poulailler Fin de course bas\",\"state_topic\":\"") + topic_limit_switch_lower +
        "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"door\","
        "\"unique_id\":\"poulailler_limit_lower_001\"" + suffix);

    publishDiscovery("binary_sensor", "poulailler_button",
        String("{\"name\":\"Poulailler Bouton poussoir\",\"state_topic\":\"") + topic_button +
        "\",\"payload_on\":\"ON\",\"payload_off\":\"OFF\","
        "\"unique_id\":\"poulailler_button_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_last_event",
        String("{\"name\":\"Poulailler Dernier evenement\",\"state_topic\":\"") + topic_last_event +
        "\",\"unique_id\":\"poulailler_last_event_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_alert",
        String("{\"name\":\"Poulailler Alerte\",\"state_topic\":\"") + topic_alert +
        "\",\"unique_id\":\"poulailler_alert_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_sunrise",
        String("{\"name\":\"Poulailler Heure d'ouverture\",\"state_topic\":\"") + topic_sunrise +
        "\",\"icon\":\"mdi:weather-sunset-up\",\"unique_id\":\"poulailler_sunrise_001\"" + suffix);

    publishDiscovery("sensor", "poulailler_sunset",
        String("{\"name\":\"Poulailler Heure de fermeture\",\"state_topic\":\"") + topic_sunset +
        "\",\"icon\":\"mdi:weather-sunset-down\",\"unique_id\":\"poulailler_sunset_001\"" + suffix);

    // Temporisations réglables depuis Home Assistant, conservées en NVS
    publishDiscovery("number", "poulailler_sunrise_offset",
        String("{\"name\":\"Poulailler Decalage ouverture\",\"state_topic\":\"") + topic_sunrise_offset +
        "\",\"command_topic\":\"" + topic_sunrise_offset_set +
        "\",\"min\":" + TIME_OFFSET_MIN + ",\"max\":" + TIME_OFFSET_MAX +
        ",\"step\":5,\"unit_of_measurement\":\"min\",\"mode\":\"box\","
        "\"entity_category\":\"config\",\"icon\":\"mdi:timer-outline\","
        "\"unique_id\":\"poulailler_sunrise_offset_001\"" + suffix);

    publishDiscovery("number", "poulailler_sunset_offset",
        String("{\"name\":\"Poulailler Decalage fermeture\",\"state_topic\":\"") + topic_sunset_offset +
        "\",\"command_topic\":\"" + topic_sunset_offset_set +
        "\",\"min\":" + TIME_OFFSET_MIN + ",\"max\":" + TIME_OFFSET_MAX +
        ",\"step\":5,\"unit_of_measurement\":\"min\",\"mode\":\"box\","
        "\"entity_category\":\"config\",\"icon\":\"mdi:timer-outline\","
        "\"unique_id\":\"poulailler_sunset_offset_001\"" + suffix);
}

// --- Publication des réglages et horaires effectifs ---
void MQTTManager::publishSettings() {
    if (mqttMutex == nullptr) return;
    if (xSemaphoreTakeRecursive(mqttMutex, pdMS_TO_TICKS(500)) != pdTRUE) return;

    if (mqttClient.connected()) {
        char buffer[8];
        mqttClient.publish(topic_firmware, FIRMWARE_VERSION, true);

        snprintf(buffer, sizeof(buffer), "%d", TimeManager::getSunriseOffset());
        mqttClient.publish(topic_sunrise_offset, buffer, true);
        snprintf(buffer, sizeof(buffer), "%d", TimeManager::getSunsetOffset());
        mqttClient.publish(topic_sunset_offset, buffer, true);

        char formatted[6];
        TimeManager::formatTime(TimeManager::getSunriseMinutes(), formatted, sizeof(formatted));
        mqttClient.publish(topic_sunrise, formatted, true);
        TimeManager::formatTime(TimeManager::getSunsetMinutes(), formatted, sizeof(formatted));
        mqttClient.publish(topic_sunset, formatted, true);
    }

    xSemaphoreGiveRecursive(mqttMutex);
}

void MQTTManager::publishTopic(const char *logicalTopic, const char *value) {
    const char *topic = nullptr;
    if (strcmp(logicalTopic, "topic_limit_switch_upper") == 0) topic = topic_limit_switch_upper;
    else if (strcmp(logicalTopic, "topic_limit_switch_lower") == 0) topic = topic_limit_switch_lower;
    else if (strcmp(logicalTopic, "topic_door_position") == 0) topic = topic_door_position;
    else if (strcmp(logicalTopic, "topic_last_event") == 0) topic = topic_last_event;
    else if (strcmp(logicalTopic, "topic_button") == 0) topic = topic_button;
    else if (strcmp(logicalTopic, "topic_alert") == 0) topic = topic_alert;

    if (topic == nullptr) {
        // Ne pas retomber sur le nom logique : cela publierait sur un topic
        // littéral « topic_xxx » et masquerait la faute de frappe.
        Serial.printf("Topic logique inconnu : %s\n", logicalTopic);
        return;
    }
    if (mqttMutex == nullptr) return;

    // Attente courte : cette fonction est appelée depuis la boucle de sécurité
    // de la porte, qui ne doit jamais être retardée par le réseau. Une
    // publication perdue est rattrapée par la republication sur reconnexion.
    if (xSemaphoreTakeRecursive(mqttMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (mqttClient.connected()) {
            mqttClient.publish(topic, value, true);
        }
        xSemaphoreGiveRecursive(mqttMutex);
    }
}
