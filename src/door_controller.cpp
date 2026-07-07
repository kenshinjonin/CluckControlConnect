/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.cpp
 * -------------------
 * Gère le contrôle de la porte automatique du poulailler :
 *   - Initialisation du matériel (broches, moteurs, fins de course)
 *   - Commandes d'ouverture, fermeture, arrêt
 *   - Gestion du bouton poussoir
 *   - Publication des états sur MQTT
 *   - Tâche FreeRTOS d'asservissement jour/nuit selon le lever/coucher du soleil
 */

#include "door_controller.h"
#include "config.h"
#include <Arduino.h>
#include "time_manager.h"
#include "mqtt_manager.h"
#include <esp_task_wdt.h>

// --- Variables d'état internes ---
static int prevUpperLimit = -1;
static int prevLowerLimit = -1;
static int prevButtonState = -1;
static String prevDoorPosition = "";

bool DoorController::isDoorMoving = false;
bool DoorController::previousDoorDirectionUp = true;
unsigned long DoorController::lastButtonPressTimestamp = 0;
unsigned long DoorController::doorMovementStartTimestamp = 0;
String DoorController::lastEvent = "-";
String DoorController::lastAlert = "-";
bool DoorController::manualOverride = false;
bool DoorController::lastDaytime = false;
bool DoorController::daytimeInitialized = false;
bool DoorController::limitSwitchFault = false;
int DoorController::limitFaultReadCount = 0;

// --- Publication MQTT de tous les états de la porte ---
void publishAllDoorStates() {
    // Fins de course
    int upper = digitalRead(config.upperLimitSwitchPinNumber);
    int lower = digitalRead(config.lowerLimitSwitchPinNumber);
    if (upper != prevUpperLimit) {
        MQTTManager::publishTopic("topic_limit_switch_upper", upper == LOW ? "ON" : "OFF");
        prevUpperLimit = upper;
    }
    if (lower != prevLowerLimit) {
        MQTTManager::publishTopic("topic_limit_switch_lower", lower == LOW ? "ON" : "OFF");
        prevLowerLimit = lower;
    }
    // Position porte (OPENING/CLOSING pour le cover Home Assistant)
    String pos;
    if (DoorController::getIsDoorMoving()) {
        pos = DoorController::getIsDirectionUp() ? "OPENING" : "CLOSING";
    } else {
        pos = upper == LOW ? "OPEN" : (lower == LOW ? "CLOSED" : "UNKNOWN");
    }
    if (pos != prevDoorPosition) {
        MQTTManager::publishTopic("topic_door_position", pos.c_str());
        prevDoorPosition = pos;
    }
}

// --- Initialisation du contrôleur de porte ---
void DoorController::init() {
    pinMode(config.buttonPinNumber, INPUT_PULLUP);
    pinMode(config.motorEnablePinNumber, OUTPUT);
    pinMode(config.motorInput1PinNumber, OUTPUT);
    pinMode(config.motorInput2PinNumber, OUTPUT);
    pinMode(config.upperLimitSwitchPinNumber, INPUT_PULLUP);
    pinMode(config.lowerLimitSwitchPinNumber, INPUT_PULLUP);

    stop();
}

// --- Commande d'ouverture de la porte ---
void DoorController::open() {
    if (!isDoorMoving && !limitSwitchFault) {
        digitalWrite(config.motorInput1PinNumber, HIGH);
        digitalWrite(config.motorInput2PinNumber, LOW);
        digitalWrite(config.motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = true;
        doorMovementStartTimestamp = millis();
        lastEvent = "OPEN";
        MQTTManager::publishDoorState("OPEN");
        MQTTManager::publishTopic("topic_last_event", "OPEN");
        publishAllDoorStates();
    }
}

// --- Commande de fermeture de la porte ---
void DoorController::close() {
    if (!isDoorMoving && !limitSwitchFault) {
        digitalWrite(config.motorInput1PinNumber, LOW);
        digitalWrite(config.motorInput2PinNumber, HIGH);
        digitalWrite(config.motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = false;
        doorMovementStartTimestamp = millis();
        lastEvent = "CLOSE";
        MQTTManager::publishDoorState("CLOSE");
        MQTTManager::publishTopic("topic_last_event", "CLOSE");
        publishAllDoorStates();
    }
}

// --- Arrêt du moteur de la porte ---
void DoorController::stop() {
    digitalWrite(config.motorInput1PinNumber, LOW);
    digitalWrite(config.motorInput2PinNumber, LOW);
    digitalWrite(config.motorEnablePinNumber, LOW);
    isDoorMoving = false;
    lastEvent = "STOP";
    MQTTManager::publishDoorState("STOP");
    MQTTManager::publishTopic("topic_last_event", "STOP");
    publishAllDoorStates();
}

// --- Suspension de l'automatisme suite à une commande manuelle ---
// L'automatisme reprend la main à la prochaine transition jour/nuit.
void DoorController::setManualOverride() {
    manualOverride = true;
}

// --- Gestion du bouton poussoir (anti-rebond, logique d'ouverture/fermeture) ---
void DoorController::handleButtonPress() {
    int buttonState = digitalRead(config.buttonPinNumber);
    if (buttonState != prevButtonState) {
        MQTTManager::publishTopic("topic_button", buttonState == LOW ? "ON" : "OFF");
        prevButtonState = buttonState;
    }
    if (buttonState == LOW && millis() - lastButtonPressTimestamp >= config.buttonDebounceDelay) {
        lastButtonPressTimestamp = millis();
        Serial.println("Appui sur le bouton detecte");
        setManualOverride();
        if (isDoorMoving) {
            // Porte en mouvement : le bouton sert d'arrêt d'urgence
            stop();
        } else {
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
    }
    publishAllDoorStates();
}

// --- Tâche FreeRTOS : asservissement jour/nuit de la porte ---
// Logique basée sur l'état plutôt que sur l'instant exact du lever/coucher :
// un événement manqué (reboot, tâche retardée) est rattrapé au cycle suivant.
// Une commande manuelle suspend l'automatisme jusqu'à la prochaine transition jour/nuit.
void DoorController::doorScheduleTask(void *parameter) {
    esp_task_wdt_add(NULL); // Tâche surveillée par le watchdog
    for(;;) {
        esp_task_wdt_reset();
        int sunriseTime = TimeManager::getSunriseMinutes();
        int sunsetTime = TimeManager::getSunsetMinutes();

        // Horaires pas encore calculés (RTC absent, éphémérides invalides) : ne rien faire
        if (sunriseTime > 0 && sunsetTime > 0) {
            int currentTime = TimeManager::getCurrentTimeMinutes();
            bool daytime = (currentTime >= sunriseTime && currentTime < sunsetTime);

            if (!daytimeInitialized) {
                lastDaytime = daytime;
                daytimeInitialized = true;
            }
            if (daytime != lastDaytime) {
                // Transition jour/nuit : l'automatisme reprend la main
                manualOverride = false;
                lastDaytime = daytime;
            }

            if (!manualOverride && !isDoorMoving) {
                bool doorOpen = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
                bool doorClosed = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;
                if (daytime && !doorOpen) {
                    open();
                } else if (!daytime && !doorClosed) {
                    close();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30000)); // Vérification toutes les 30 secondes
    }
}

// --- Tâche FreeRTOS : gestion du bouton poussoir ---
void DoorController::buttonTask(void *parameter) {
    esp_task_wdt_add(NULL); // Tâche surveillée par le watchdog
    for(;;) {
        esp_task_wdt_reset();
        handleButtonPress();
        checkLimitSwitchFault();
        checkDoorMovementSafety();
        vTaskDelay(pdMS_TO_TICKS(50)); // Vérification toutes les 50ms
    }
}

// --- Sécurité : coupe le moteur à l'atteinte de la fin de course ou au-delà du temps max de mouvement ---
void DoorController::checkDoorMovementSafety() {
    if (!isDoorMoving) return;

    bool upperLimitActive = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
    bool lowerLimitActive = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;

    if (previousDoorDirectionUp && upperLimitActive) {
        stop();
        return;
    }
    if (!previousDoorDirectionUp && lowerLimitActive) {
        stop();
        return;
    }

    if (millis() - doorMovementStartTimestamp >= config.maxDoorMovementDuration) {
        Serial.println("Duree max de mouvement depassee, arret de securite du moteur");
        stop();
        lastAlert = "MOVEMENT_TIMEOUT";
        MQTTManager::publishTopic("topic_alert", "MOVEMENT_TIMEOUT");
        // Suspend l'automatisme pour ne pas relancer le moteur en boucle sur un blocage
        // (une nouvelle tentative aura lieu à la prochaine transition jour/nuit ou sur commande manuelle)
        setManualOverride();
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
            lastAlert = "LIMIT_SWITCH_FAULT";
            MQTTManager::publishTopic("topic_alert", "LIMIT_SWITCH_FAULT");
            Serial.println("Panne fins de course : haut et bas actifs simultanement");
        }
    } else {
        limitFaultReadCount = 0;
        if (limitSwitchFault) {
            limitSwitchFault = false;
            lastAlert = "-";
            MQTTManager::publishTopic("topic_alert", "-");
            Serial.println("Panne fins de course disparue, mouvements reautorises");
        }
    }
}
