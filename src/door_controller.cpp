// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include "door_controller.h"
#include "config.h"
#include <Arduino.h>
#include "time_manager.h"
#include "mqtt_manager.h"

// Variables d'état précédentes pour détection de changement
static int prevUpperLimit = -1;
static int prevLowerLimit = -1;
static int prevButtonState = -1;
static String prevDoorPosition = "";

bool DoorController::isDoorMoving = false;
bool DoorController::previousDoorDirectionUp = true;
unsigned long DoorController::lastButtonPressTimestamp = 0;

void publishAllDoorStates() {
    // Fins de course
    int upper = digitalRead(upperLimitSwitchPinNumber);
    int lower = digitalRead(lowerLimitSwitchPinNumber);
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

void DoorController::init() {
    pinMode(buttonPinNumber, INPUT_PULLUP);
    pinMode(motorEnablePinNumber, OUTPUT);
    pinMode(motorInput1PinNumber, OUTPUT);
    pinMode(motorInput2PinNumber, OUTPUT);
    pinMode(upperLimitSwitchPinNumber, INPUT_PULLUP);
    pinMode(lowerLimitSwitchPinNumber, INPUT_PULLUP);
    
    stop();
}

void DoorController::open() {
    if (!isDoorMoving) {
        digitalWrite(motorInput1PinNumber, HIGH);
        digitalWrite(motorInput2PinNumber, LOW);
        digitalWrite(motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = true;
        MQTTManager::publishDoorState("OPEN");
        MQTTManager::publishTopic("topic_last_event", "OPEN");
        publishAllDoorStates();
    }
}

void DoorController::close() {
    if (!isDoorMoving) {
        digitalWrite(motorInput1PinNumber, LOW);
        digitalWrite(motorInput2PinNumber, HIGH);
        digitalWrite(motorEnablePinNumber, HIGH);
        isDoorMoving = true;
        previousDoorDirectionUp = false;
        MQTTManager::publishDoorState("CLOSE");
        MQTTManager::publishTopic("topic_last_event", "CLOSE");
        publishAllDoorStates();
    }
}

void DoorController::stop() {
    digitalWrite(motorInput1PinNumber, LOW);
    digitalWrite(motorInput2PinNumber, LOW);
    digitalWrite(motorEnablePinNumber, LOW);
    isDoorMoving = false;
    MQTTManager::publishDoorState("STOP");
    MQTTManager::publishTopic("topic_last_event", "STOP");
    publishAllDoorStates();
}

void DoorController::handleButtonPress() {
    int buttonState = digitalRead(buttonPinNumber);
    if (buttonState != prevButtonState) {
        MQTTManager::publishTopic("topic_button", buttonState == LOW ? "ON" : "OFF");
        prevButtonState = buttonState;
    }
    if (buttonState == LOW && millis() - lastButtonPressTimestamp >= buttonDebounceDelay) {
        lastButtonPressTimestamp = millis();
        Serial.println("Appui sur le bouton détecté");
        if (!isDoorMoving) {
            bool motorInput1State = digitalRead(motorInput1PinNumber);
            bool motorInput2State = digitalRead(motorInput2PinNumber);
            if (motorInput1State || motorInput2State) {
                stop();
            } else {
                bool upperLimitActive = digitalRead(upperLimitSwitchPinNumber) == LOW;
                bool lowerLimitActive = digitalRead(lowerLimitSwitchPinNumber) == LOW;
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

void DoorController::buttonTask(void *parameter) {
    for(;;) {
        handleButtonPress();
        vTaskDelay(pdMS_TO_TICKS(50)); // Vérification toutes les 50ms
    }
}