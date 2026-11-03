/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.cpp
 * -------------------
 * Gère le contrôle de la porte automatique du poulailler :
 *   - Initialisation du matériel (broches, moteur, fins de course)
 *   - File de commandes : le moteur n'est piloté que par doorControlTask
 *   - Sécurités : fins de course, durée max de mouvement, panne de fin de course
 *   - Publication des états sur MQTT
 *   - Tâche d'asservissement jour/nuit selon le lever/coucher du soleil
 */

#include "door_controller.h"
#include "config.h"
#include "task_utils.h"
#include "daytime.h"
#include <Arduino.h>
#include "time_manager.h"
#include "mqtt_manager.h"

// --- File de commandes (seule interface d'écriture depuis les autres tâches) ---
struct DoorRequest {
    DoorCommand cmd;
    bool manual;
};
static QueueHandle_t commandQueue = nullptr;

// --- Cache de publication : touché uniquement par doorControlTask ---
static int prevUpperLimit = -1;
static int prevLowerLimit = -1;
static int prevButtonState = -1;
static const char *prevDoorPosition = nullptr;

const char *volatile DoorController::lastEvent = "-";
const char *volatile DoorController::lastAlert = "-";

volatile bool DoorController::isDoorMoving = false;
volatile bool DoorController::previousDoorDirectionUp = true;
volatile bool DoorController::limitSwitchFault = false;
volatile bool DoorController::manualOverride = false;
volatile bool DoorController::forceRepublish = false;

unsigned long DoorController::lastButtonPressTimestamp = 0;
unsigned long DoorController::doorMovementStartTimestamp = 0;
unsigned long DoorController::lastMotorStopTimestamp = 0;
unsigned long DoorController::automationPausedSince = 0;
bool DoorController::automationPaused = false;
uint8_t DoorController::movementFailureCount = 0;
int DoorController::limitFaultReadCount = 0;
bool DoorController::lastDaytime = false;
bool DoorController::daytimeInitialized = false;

// --- Accesseurs ---
bool DoorController::getIsDoorMoving() { return isDoorMoving; }
const char *DoorController::getLastEvent() { return lastEvent; }
const char *DoorController::getLastAlert() { return lastAlert; }

// Position déduite des fins de course : lecture GPIO pure, appelable depuis n'importe quelle tâche
const char *DoorController::getDoorPosition() {
    if (isDoorMoving) {
        return previousDoorDirectionUp ? "OPENING" : "CLOSING";
    }
    if (digitalRead(config.upperLimitSwitchPinNumber) == LOW) return "OPEN";
    if (digitalRead(config.lowerLimitSwitchPinNumber) == LOW) return "CLOSED";
    return "UNKNOWN";
}

// --- Initialisation du contrôleur de porte ---
void DoorController::init() {
    pinMode(config.buttonPinNumber, INPUT_PULLUP);
    pinMode(config.upperLimitSwitchPinNumber, INPUT_PULLUP);
    pinMode(config.lowerLimitSwitchPinNumber, INPUT_PULLUP);

    // Sorties moteur mises à l'état bas avant d'être configurées en sortie :
    // évite toute impulsion parasite sur le pont en H pendant l'initialisation.
    digitalWrite(config.motorEnablePinNumber, LOW);
    digitalWrite(config.motorInput1PinNumber, LOW);
    digitalWrite(config.motorInput2PinNumber, LOW);
    pinMode(config.motorEnablePinNumber, OUTPUT);
    pinMode(config.motorInput1PinNumber, OUTPUT);
    pinMode(config.motorInput2PinNumber, OUTPUT);

    commandQueue = xQueueCreate(8, sizeof(DoorRequest));

    stop();
}

// --- Dépôt d'une commande dans la file ---
bool DoorController::requestCommand(DoorCommand cmd, bool manual) {
    if (commandQueue == nullptr) return false;
    DoorRequest request = {cmd, manual};
    if (xQueueSend(commandQueue, &request, 0) != pdTRUE) {
        Serial.println("File de commandes pleine, commande ignoree");
        return false;
    }
    return true;
}

void DoorController::forceStatePublish() {
    forceRepublish = true;
}

// --- Publication MQTT des états (appelée uniquement depuis doorControlTask) ---
void DoorController::publishAllDoorStates() {
    if (forceRepublish) {
        // Reconnexion MQTT : le cache est vidé pour tout republier
        prevUpperLimit = -1;
        prevLowerLimit = -1;
        prevButtonState = -1;
        prevDoorPosition = nullptr;
        forceRepublish = false;
    }

    int upper = digitalRead(config.upperLimitSwitchPinNumber);
    int lower = digitalRead(config.lowerLimitSwitchPinNumber);
    if (upper != prevUpperLimit) {
        MQTTManager::publishTopic(topic_limit_switch_upper, upper == LOW ? "ON" : "OFF");
        prevUpperLimit = upper;
    }
    if (lower != prevLowerLimit) {
        MQTTManager::publishTopic(topic_limit_switch_lower, lower == LOW ? "ON" : "OFF");
        prevLowerLimit = lower;
    }

    const char *pos = getDoorPosition();
    if (prevDoorPosition == nullptr || strcmp(pos, prevDoorPosition) != 0) {
        MQTTManager::publishTopic(topic_door_position, pos);
        prevDoorPosition = pos;
    }
}

void DoorController::setEvent(const char *event) {
    lastEvent = event;
    MQTTManager::publishTopic(topic_last_event, event);
}

void DoorController::setAlert(const char *alert) {
    lastAlert = alert;
    MQTTManager::publishTopic(topic_alert, alert);
}

// --- Temps mort avant réalimentation du moteur ---
// Inverser le sens sans phase de roue libre soumet le pont en H au courant de
// démarrage additionné de la force contre-électromotrice du moteur encore
// lancé : c'est le mode de destruction classique d'un L298N.
void DoorController::applyDeadTime() {
    unsigned long elapsed = millis() - lastMotorStopTimestamp;
    if (elapsed < config.motorReverseDeadTime) {
        vTaskDelay(pdMS_TO_TICKS(config.motorReverseDeadTime - elapsed));
    }
}

// --- Commande d'ouverture de la porte ---
void DoorController::open() {
    if (limitSwitchFault) return;
    if (isDoorMoving) {
        if (previousDoorDirectionUp) return; // Déjà en ouverture
        stop(); // Inversion demandée : le temps mort s'applique ci-dessous
    }
    // Déjà ouverte : démarrer ferait forcer le moteur contre la butée un cycle
    if (digitalRead(config.upperLimitSwitchPinNumber) == LOW) return;
    applyDeadTime();
    digitalWrite(config.motorInput1PinNumber, HIGH);
    digitalWrite(config.motorInput2PinNumber, LOW);
    digitalWrite(config.motorEnablePinNumber, HIGH); // Alimentation en dernier
    isDoorMoving = true;
    previousDoorDirectionUp = true;
    doorMovementStartTimestamp = millis();
    setEvent("OPEN");
    publishAllDoorStates();
}

// --- Commande de fermeture de la porte ---
void DoorController::close() {
    if (limitSwitchFault) return;
    if (isDoorMoving) {
        if (!previousDoorDirectionUp) return; // Déjà en fermeture
        stop(); // Inversion demandée : le temps mort s'applique ci-dessous
    }
    if (digitalRead(config.lowerLimitSwitchPinNumber) == LOW) return; // Déjà fermée
    applyDeadTime();
    digitalWrite(config.motorInput1PinNumber, LOW);
    digitalWrite(config.motorInput2PinNumber, HIGH);
    digitalWrite(config.motorEnablePinNumber, HIGH); // Alimentation en dernier
    isDoorMoving = true;
    previousDoorDirectionUp = false;
    doorMovementStartTimestamp = millis();
    setEvent("CLOSE");
    publishAllDoorStates();
}

// --- Arrêt du moteur de la porte ---
void DoorController::stop() {
    digitalWrite(config.motorEnablePinNumber, LOW); // Coupure de l'alimentation en premier
    digitalWrite(config.motorInput1PinNumber, LOW);
    digitalWrite(config.motorInput2PinNumber, LOW);
    lastMotorStopTimestamp = millis();
    isDoorMoving = false;
    setEvent("STOP");
    publishAllDoorStates();
}

// --- Gestion du bouton poussoir ---
void DoorController::handleButtonPress() {
    int buttonState = digitalRead(config.buttonPinNumber);

    if (buttonState != prevButtonState) {
        MQTTManager::publishTopic(topic_button, buttonState == LOW ? "ON" : "OFF");
    }

    // Déclenchement sur front descendant franc uniquement. Un bouton maintenu,
    // un contact collé ou un fil pincé (humidité dans le boîtier) doit rester
    // sans effet : sur un simple test de niveau, il relancerait une commande à
    // chaque période d'anti-rebond et ferait cycler le moteur indéfiniment.
    bool fallingEdge = (buttonState == LOW && prevButtonState == HIGH);
    prevButtonState = buttonState;

    if (!fallingEdge) return;
    if (millis() - lastButtonPressTimestamp < config.buttonDebounceDelay) return;
    lastButtonPressTimestamp = millis();

    Serial.println("Appui sur le bouton detecte");
    manualOverride = true;

    if (isDoorMoving) {
        // Porte en mouvement : le bouton sert d'arrêt d'urgence
        stop();
        return;
    }

    bool upperLimitActive = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
    bool lowerLimitActive = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;
    if (upperLimitActive) {
        close();
    } else if (lowerLimitActive) {
        open();
    } else {
        // Porte arrêtée à mi-course : repartir dans le sens inverse du dernier mouvement
        previousDoorDirectionUp ? close() : open();
    }
}

// --- Sécurité : coupe le moteur en fin de course ou au-delà de la durée max ---
void DoorController::checkDoorMovementSafety() {
    if (!isDoorMoving) return;

    bool upperLimitActive = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
    bool lowerLimitActive = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;

    // Arrêt immédiat, sans confirmation sur plusieurs lectures : un arrêt
    // intempestif est rattrapé au cycle suivant de l'asservissement, alors
    // qu'un arrêt retardé laisse le moteur forcer contre la butée.
    if ((previousDoorDirectionUp && upperLimitActive) ||
        (!previousDoorDirectionUp && lowerLimitActive)) {
        stop();
        movementFailureCount = 0;
        automationPaused = false;
        if (strcmp((const char *)lastAlert, "MOVEMENT_TIMEOUT") == 0 ||
            strcmp((const char *)lastAlert, "DOOR_JAMMED") == 0) {
            setAlert("-");
        }
        return;
    }

    if (millis() - doorMovementStartTimestamp >= config.maxDoorMovementDuration) {
        Serial.println("Duree max de mouvement depassee, arret de securite du moteur");
        stop();
        movementFailureCount++;
        if (movementFailureCount >= config.maxMovementFailures) {
            // Blocage avéré : on cesse de solliciter le moteur et on laisse
            // Home Assistant alerter. L'automatisme reprendra à la prochaine
            // transition jour/nuit ou sur commande manuelle.
            manualOverride = true;
            setAlert("DOOR_JAMMED");
        } else {
            // Obstacle passager (branche, givre) : nouvelle tentative différée
            automationPaused = true;
            automationPausedSince = millis();
            setAlert("MOVEMENT_TIMEOUT");
        }
    }
}

// --- Détection de panne : les deux fins de course actives simultanément ---
// Physiquement impossible sur une porte : câble en court-circuit ou interrupteur HS.
// Trois lectures consécutives en défaut (150 ms) sont exigées pour écarter un
// parasite ; tant que la panne est présente, tout mouvement est refusé.
void DoorController::checkLimitSwitchFault() {
    bool upperLimitActive = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
    bool lowerLimitActive = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;

    if (upperLimitActive && lowerLimitActive) {
        if (limitFaultReadCount < 3) limitFaultReadCount++;
        if (limitFaultReadCount >= 3 && !limitSwitchFault) {
            limitSwitchFault = true;
            if (isDoorMoving) {
                stop();
            }
            setAlert("LIMIT_SWITCH_FAULT");
            Serial.println("Panne fins de course : haut et bas actifs simultanement");
        }
    } else {
        limitFaultReadCount = 0;
        if (limitSwitchFault) {
            limitSwitchFault = false;
            setAlert("-");
            Serial.println("Panne fins de course disparue, mouvements reautorises");
        }
    }
}

// --- Tâche FreeRTOS : pilotage moteur, bouton et sécurités ---
// Seule tâche autorisée à écrire sur les sorties du pont en H.
void DoorController::doorControlTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();

        checkLimitSwitchFault();
        handleButtonPress();

        DoorRequest request;
        while (commandQueue != nullptr && xQueueReceive(commandQueue, &request, 0) == pdTRUE) {
            if (request.manual) {
                manualOverride = true;
            }
            switch (request.cmd) {
                case DoorCommand::Open:  open();  break;
                case DoorCommand::Close: close(); break;
                case DoorCommand::Stop:  stop();  break;
            }
        }

        checkDoorMovementSafety();
        publishAllDoorStates();

        vTaskDelay(pdMS_TO_TICKS(50)); // Vérification toutes les 50 ms
    }
}

// --- Tâche FreeRTOS : asservissement jour/nuit de la porte ---
// Logique basée sur l'état plutôt que sur l'instant exact du lever/coucher :
// un événement manqué (reboot, tâche retardée) est rattrapé au cycle suivant.
// Une commande manuelle suspend l'automatisme jusqu'à la prochaine transition.
void DoorController::doorScheduleTask(void *parameter) {
    esp_task_wdt_add(NULL);
    for (;;) {
        esp_task_wdt_reset();

        int sunriseTime = TimeManager::getSunriseMinutes();
        int sunsetTime = TimeManager::getSunsetMinutes();
        int currentTime = TimeManager::getCurrentTimeMinutes();

        // Horaires indisponibles (RTC absente, date ou éphémérides invalides) :
        // l'automatisme reste gelé plutôt que d'agir sur une heure fantaisiste.
        if (sunriseTime >= 0 && sunsetTime >= 0 && currentTime >= 0) {
            bool daytime = isDaytime(sunriseTime, sunsetTime, currentTime);

            if (!daytimeInitialized) {
                lastDaytime = daytime;
                daytimeInitialized = true;
            }
            if (daytime != lastDaytime) {
                // Transition jour/nuit : l'automatisme reprend la main
                manualOverride = false;
                automationPaused = false;
                movementFailureCount = 0;
                lastDaytime = daytime;
            }

            if (automationPaused && millis() - automationPausedSince >= config.automationRetryDelay) {
                automationPaused = false;
            }

            if (!manualOverride && !automationPaused && !isDoorMoving && !limitSwitchFault) {
                bool doorOpen = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
                bool doorClosed = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;
                if (daytime && !doorOpen) {
                    requestCommand(DoorCommand::Open, false);
                } else if (!daytime && !doorClosed) {
                    requestCommand(DoorCommand::Close, false);
                }
            }
        }

        watchdogDelay(30000); // Vérification toutes les 30 secondes
    }
}
