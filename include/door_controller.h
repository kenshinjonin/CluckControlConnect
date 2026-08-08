/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.h
 * -----------------
 * Déclaration de la classe DoorController.
 *
 * Modèle de concurrence : le moteur n'est piloté que par doorControlTask
 * (écrivain unique). Toutes les autres sources de commande — MQTT, interface
 * web, asservissement jour/nuit — passent par requestCommand(), qui dépose la
 * demande dans une file FreeRTOS. Cela supprime toute course sur les sorties du
 * pont en H sans introduire de verrou supplémentaire, et garantit qu'aucune
 * tâche réseau ne peut retarder la boucle de sécurité.
 */

#pragma once

#include <Arduino.h>

// Commandes acceptées par la file
enum class DoorCommand : uint8_t {
    Open,
    Close,
    Stop
};

class DoorController {
public:
    static void init();

    // Dépose une commande dans la file. `manual` à true suspend l'asservissement
    // jour/nuit jusqu'à la prochaine transition. Retourne false si la file est pleine.
    static bool requestCommand(DoorCommand cmd, bool manual);

    // Demande la republication de tous les états (après une reconnexion MQTT)
    static void forceStatePublish();

    static void doorScheduleTask(void *parameter);
    static void doorControlTask(void *parameter);

    static bool getIsDoorMoving();
    static bool getIsDirectionUp();
    static const char *getLastEvent();
    static const char *getLastAlert();
    static const char *getDoorPosition();

private:
    // Pilotage moteur : strictement réservé à doorControlTask
    static void open();
    static void close();
    static void stop();
    static void applyDeadTime();

    static void handleButtonPress();
    static void checkDoorMovementSafety();
    static void checkLimitSwitchFault();
    static void publishAllDoorStates();
    static void setEvent(const char *event);
    static void setAlert(const char *alert);

    // Les valeurs affectées à lastEvent/lastAlert sont exclusivement des
    // littéraux : l'écriture se réduit à celle d'un pointeur, ce qui reste sûr
    // quand le serveur web les lit depuis l'autre cœur (une String ne le serait
    // pas, son affectation libérant le tampon en cours de lecture).
    static const char *volatile lastEvent;
    static const char *volatile lastAlert;

    static volatile bool isDoorMoving;
    static volatile bool previousDoorDirectionUp;
    static volatile bool limitSwitchFault;
    static volatile bool manualOverride;
    static volatile bool forceRepublish;

    static unsigned long lastButtonPressTimestamp;
    static unsigned long doorMovementStartTimestamp;
    static unsigned long lastMotorStopTimestamp;
    static unsigned long automationPausedSince;
    static bool automationPaused;
    static uint8_t movementFailureCount;
    static int limitFaultReadCount;
    static bool lastDaytime;
    static bool daytimeInitialized;
};
