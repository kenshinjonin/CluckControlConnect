/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * door_controller.h
 * -----------------
 * Déclaration de la classe DoorController pour la gestion de la porte automatique :
 *   - Initialisation du matériel
 *   - Commandes d'ouverture, fermeture, arrêt
 *   - Gestion du bouton poussoir
 *   - Tâches FreeRTOS pour l'automatisation
 */

#pragma once

class DoorController {
public:
    static void init();                        // Initialisation du contrôleur
    static void open();                        // Ouvre la porte
    static void close();                       // Ferme la porte
    static void stop();                        // Arrête le moteur
    static void handleButtonPress();           // Gère l'appui sur le bouton
    static void openDoorTask(void *parameter); // Tâche FreeRTOS : ouverture automatique
    static void closeDoorTask(void *parameter);// Tâche FreeRTOS : fermeture automatique
    static void buttonTask(void *parameter);   // Tâche FreeRTOS : gestion bouton
    static bool getIsDoorMoving() { return isDoorMoving; } // Indique si la porte est en mouvement
    
private:
    static bool isDoorMoving;                  // État du mouvement de la porte
    static bool previousDoorDirectionUp;       // Dernière direction de la porte
    static unsigned long lastButtonPressTimestamp; // Timestamp du dernier appui bouton
};