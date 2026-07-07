/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.h
 * -----------------
 * Déclaration de la classe DoorController pour la gestion de la porte automatique :
 *   - Initialisation du matériel
 *   - Commandes d'ouverture, fermeture, arrêt
 *   - Gestion du bouton poussoir
 *   - Tâches FreeRTOS pour l'automatisation
 */

#pragma once

#include <Arduino.h>

class DoorController {
public:
    static void init();                        // Initialisation du contrôleur
    static void open();                        // Ouvre la porte
    static void close();                       // Ferme la porte
    static void stop();                        // Arrête le moteur
    static void handleButtonPress();           // Gère l'appui sur le bouton
    static void doorScheduleTask(void *parameter); // Tâche FreeRTOS : asservissement jour/nuit de la porte
    static void buttonTask(void *parameter);   // Tâche FreeRTOS : gestion bouton
    static void checkDoorMovementSafety();     // Coupe le moteur en fin de course ou en cas de dépassement du temps max
    static void setManualOverride();           // Suspend l'automatisme jusqu'à la prochaine transition jour/nuit
    static bool getIsDoorMoving() { return isDoorMoving; } // Indique si la porte est en mouvement
    static bool getIsDirectionUp() { return previousDoorDirectionUp; } // Direction du mouvement en cours (ou du dernier)
    static String getLastEvent() { return lastEvent; }     // Dernier événement porte (OPEN/CLOSE/STOP)
    static String getLastAlert() { return lastAlert; }     // Dernière alerte de sécurité

private:
    static void checkLimitSwitchFault();       // Détection de l'incohérence « deux fins de course actives »
    static bool limitSwitchFault;              // Panne fins de course détectée (mouvements refusés)
    static int limitFaultReadCount;            // Lectures consécutives en défaut (anti-parasite)
    static bool isDoorMoving;                  // État du mouvement de la porte
    static bool previousDoorDirectionUp;       // Dernière direction de la porte
    static unsigned long lastButtonPressTimestamp; // Timestamp du dernier appui bouton
    static unsigned long doorMovementStartTimestamp; // Timestamp de début du mouvement en cours
    static String lastEvent;                   // Dernier événement porte
    static String lastAlert;                   // Dernière alerte de sécurité
    static bool manualOverride;                // Automatisme suspendu suite à une commande manuelle
    static bool lastDaytime;                   // Dernier état jour/nuit connu (détection de transition)
    static bool daytimeInitialized;            // true une fois lastDaytime initialisé
};
