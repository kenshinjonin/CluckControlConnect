/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * web_server.h
 * ------------
 * Déclaration de la classe WebServerManager pour la gestion de l'interface web :
 *   - Initialisation du serveur web
 *   - Définition des routes et authentification
 *   - Tâche de gestion des requêtes et de l'OTA
 */

#pragma once

#include <WebServer.h>

class WebServerManager {
public:
    static void init();
    static void webServerTask(void *parameter); // Tâche FreeRTOS : traitement des requêtes web et OTA
    static WebServer server;

private:
    static void setupRoutes();
};
