/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.cpp
 * -------------------
 * Gère le contrôle de la porte automatique du poulailler :
 *   - Initialisation du matériel (broches, moteurs, fins de course)
 *   - Commandes d'ouverture, fermeture, arrêt
 *   - Gestion du bouton poussoir
 *   - Publication des états sur MQTT
 *   - Tâches FreeRTOS pour l'ouverture/fermeture automatique selon le lever/coucher du soleil
 */

#include "door_controller.h"
#include "config.h"
#include <Arduino.h>
#include "time_manager.h"
#include "mqtt_manager.h"

// --- Variables d'état internes ---
static int prevUpperLimit = -1;
static int prevLowerLimit = -1;
static int prevButtonState = -1;
static String prevDoorPosition = "";

bool DoorController::isDoorMoving = false;
bool DoorController::previousDoorDirectionUp = true;
unsigned long DoorController::lastButtonPressTimestamp = 0;

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
    // Position porte
    String pos = DoorController::getIsDoorMoving() ? "MOVING" : (upper == LOW ? "OPEN" : (lower == LOW ? "CLOSED" : "UNKNOWN"));
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
    if (!isDoorMoving) {
        digitalWrite(config.motorInput1PinNumber, HIGH);
        digitalWrite(config.motorInput2PinNumber, LOW);
        digitalWrite(config.motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = true;
        MQTTManager::publishDoorState("OPEN");
        MQTTManager::publishTopic("topic_last_event", "OPEN");
        publishAllDoorStates();
    }
}

// --- Commande de fermeture de la porte ---
void DoorController::close() {
    if (!isDoorMoving) {
        digitalWrite(config.motorInput1PinNumber, LOW);
        digitalWrite(config.motorInput2PinNumber, HIGH);
        digitalWrite(config.motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = false;
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
    MQTTManager::publishDoorState("STOP");
    MQTTManager::publishTopic("topic_last_event", "STOP");
    publishAllDoorStates();
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
        Serial.println("Appui sur le bouton détecté");
        if (!isDoorMoving) {
            bool motorInput1State = digitalRead(config.motorInput1PinNumber);
            bool motorInput2State = digitalRead(config.motorInput2PinNumber);
            if (motorInput1State || motorInput2State) {
                stop();
            } else {
                bool upperLimitActive = digitalRead(config.upperLimitSwitchPinNumber) == LOW;
                bool lowerLimitActive = digitalRead(config.lowerLimitSwitchPinNumber) == LOW;
                if (upperLimitActive && lowerLimitActive) {
                    previousDoorDirectionUp ? close() : open();
                } else if (upperLimitActive) {
                    close();
                } else if (lowerLimitActive) {
                    open();
                }
            }
        }
    }
    publishAllDoorStates();
}

// --- Tâche FreeRTOS : ouverture automatique au lever du soleil ---
void DoorController::openDoorTask(void *parameter) {
    for(;;) {
        int currentTime = TimeManager::getCurrentTimeMinutes();
        int sunriseTime = TimeManager::getSunriseMinutes();
        
        // Ouvrir la porte au lever du soleil
        if (currentTime == sunriseTime && !isDoorMoving) {
            open();
        }
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Vérification toutes les 30 secondes
    }
}

// --- Tâche FreeRTOS : fermeture automatique au coucher du soleil ---
void DoorController::closeDoorTask(void *parameter) {
    for(;;) {
        int currentTime = TimeManager::getCurrentTimeMinutes();
        int sunsetTime = TimeManager::getSunsetMinutes();
        
        // Fermer la porte au coucher du soleil
        if (currentTime == sunsetTime && !isDoorMoving) {
            close();
        }
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Vérification toutes les 30 secondes
    }
}

// --- Tâche FreeRTOS : gestion du bouton poussoir ---
void DoorController::buttonTask(void *parameter) {
    for(;;) {
        handleButtonPress();
        vTaskDelay(pdMS_TO_TICKS(50)); // Vérification toutes les 50ms
    }
}