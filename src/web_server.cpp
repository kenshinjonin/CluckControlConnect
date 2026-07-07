/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * web_server.cpp
 * --------------
 * Gère l'interface web embarquée du système :
 *   - Authentification HTTP basique (login/mot de passe) avec limitation
 *     des tentatives (compteur à expiration, seuls les mauvais mots de
 *     passe sont comptés)
 *   - Définition des routes (page principale, API JSON, commandes)
 *   - Génération de l'interface HTML
 *   - Intégration ElegantOTA (protégée par la même authentification)
 *   - Publication d'informations système (état, position, RSSI, etc.)
 */

#include "web_server.h"
#include <ElegantOTA.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"
#include <Arduino.h>
#include "mqtt_manager.h"
#include "door_controller.h"
#include "time_manager.h"
#include "config.h"

namespace {
// --- Paramètres d'authentification ---
const char* www_username = SECRET_WEB_USER;
const char* www_password = SECRET_WEB_PASS;
const int MAX_LOGIN_ATTEMPTS = 5;
const unsigned long LOGIN_ATTEMPTS_RESET_MS = 5 * 60 * 1000; // Expiration du compteur d'échecs
int loginAttempts = 0;
unsigned long lastFailedAttemptTimestamp = 0;

// --- Vérification de l'authentification HTTP basique ---
bool isAuthenticated(WebServer& server) {
    // Le compteur d'échecs expire : pas de verrouillage définitif jusqu'au reboot
    if (loginAttempts > 0 && millis() - lastFailedAttemptTimestamp > LOGIN_ATTEMPTS_RESET_MS) {
        loginAttempts = 0;
    }
    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        server.send(429, "text/plain", "Trop de tentatives. Reessayez dans quelques minutes.");
        return false;
    }
    if (server.authenticate(www_username, www_password)) {
        loginAttempts = 0;
        return true;
    }
    // Ne compter que les requêtes portant des identifiants (mauvais mot de passe) :
    // la première requête d'un navigateur n'a pas encore de header Authorization
    if (server.hasHeader("Authorization")) {
        loginAttempts++;
        lastFailedAttemptTimestamp = millis();
    }
    server.requestAuthentication();
    return false;
}
}

WebServer WebServerManager::server(80);

// --- Initialisation du serveur web et des routes ---
void WebServerManager::init() {
    // Nécessaire pour que hasHeader("Authorization") fonctionne dans isAuthenticated
    const char* headerKeys[] = {"Authorization"};
    server.collectHeaders(headerKeys, 1);

    setupRoutes();
    ElegantOTA.begin(&server);
    ElegantOTA.setAuth(www_username, www_password); // Protège /update (UI et upload OTA)
    server.begin();
}

// --- Gestion des requêtes entrantes ---
void WebServerManager::handleClient() {
    server.handleClient();
}

// --- Tâche FreeRTOS : traitement des requêtes web et OTA ---
void WebServerManager::webServerTask(void *parameter) {
    esp_task_wdt_add(NULL); // Tâche surveillée par le watchdog
    for(;;) {
        esp_task_wdt_reset();
        server.handleClient();
        ElegantOTA.loop(); // Requis par ElegantOTA v3 (finalise le reboot post-mise à jour)
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- Définition des routes HTTP ---
void WebServerManager::setupRoutes() {
    server.on("/", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        server.send(200, "text/html", generateHtmlResponse());
    });

    // Note : /update est servi par ElegantOTA (protégé via ElegantOTA.setAuth)

    server.on("/api/status", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        String json = "{";
        // État de la porte
        String doorState = DoorController::getIsDoorMoving() ? "MOVING" : "STOPPED";
        // Position
        int upper = digitalRead(config.upperLimitSwitchPinNumber);
        int lower = digitalRead(config.lowerLimitSwitchPinNumber);
        String doorPosition = upper == LOW ? "OPEN" : (lower == LOW ? "CLOSED" : "UNKNOWN");
        // Fins de course
        String limitUpper = upper == LOW ? "ON" : "OFF";
        String limitLower = lower == LOW ? "ON" : "OFF";
        // Bouton poussoir
        int buttonState = digitalRead(config.buttonPinNumber);
        String button = buttonState == LOW ? "ON" : "OFF";
        // Dernier événement
        String lastEvent = DoorController::getLastEvent();
        // Dernière alerte de sécurité
        String alert = DoorController::getLastAlert();
        json += "\"doorState\":\"" + doorState + "\",";
        json += "\"doorPosition\":\"" + doorPosition + "\",";
        json += "\"limitUpper\":\"" + limitUpper + "\",";
        json += "\"limitLower\":\"" + limitLower + "\",";
        json += "\"buttonState\":\"" + button + "\",";
        json += "\"wifiRssi\":\"" + String(WiFi.RSSI()) + "\",";
        json += "\"uptime\":\"" + String(millis()/1000) + "\",";
        json += "\"lastEvent\":\"" + lastEvent + "\",";
        json += "\"alert\":\"" + alert + "\"}";
        server.send(200, "application/json", json);
    });

    server.on("/cmd", HTTP_POST, []() {
        if (!isAuthenticated(server)) return;
        if (server.hasArg("action")) {
            String action = server.arg("action");
            DoorController::setManualOverride();
            if (action == "OPEN") DoorController::open();
            else if (action == "CLOSE") DoorController::close();
            else if (action == "STOP") DoorController::stop();
        }
        server.sendHeader("Location", "/");
        server.send(303);
    });
}

// --- Génération de la page HTML principale ---
String WebServerManager::generateHtmlResponse() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Poulailler - Contrôle de la Porte</title>";
    html += "<style>body{font-family:sans-serif;background:#f4f4f4;margin:0;padding:0;}h1{background:#4CAF50;color:#fff;padding:10px;}section{margin:20px;}button{padding:10px 20px;margin:5px;font-size:16px;}div.status{margin:10px 0;padding:10px;background:#fff;border-radius:5px;box-shadow:0 2px 4px #ccc;}footer{margin:20px;text-align:center;color:#888;}</style>";
    html += "</head><body>";
    html += "<h1>Poulailler - Contrôle de la Porte</h1>";
    html += "<section>";
    html += "<div class='status'><b>État de la porte :</b> <span id='doorState'>?</span></div>";
    html += "<div class='status'><b>Position :</b> <span id='doorPosition'>?</span></div>";
    html += "<div class='status'><b>Fin de course haut :</b> <span id='limitUpper'>?</span> | <b>Fin de course bas :</b> <span id='limitLower'>?</span></div>";
    html += "<div class='status'><b>Bouton poussoir :</b> <span id='buttonState'>?</span></div>";
    html += "<div class='status'><b>RSSI WiFi :</b> <span id='wifiRssi'>?</span> dBm</div>";
    html += "<div class='status'><b>Uptime :</b> <span id='uptime'>?</span> s</div>";
    html += "<div class='status'><b>Dernier événement :</b> <span id='lastEvent'>?</span></div>";
    html += "<div class='status'><b>Alerte :</b> <span id='alert'>?</span></div>";
    html += "<form method='POST' action='/cmd'><button name='action' value='OPEN'>Ouvrir</button><button name='action' value='CLOSE'>Fermer</button><button name='action' value='STOP'>Stop</button></form>";
    html += "<p><a href='/update'>Mise à jour OTA</a></p>";
    html += "</section>";
    html += "<footer>CluckControl Connect © 2025</footer>";
    html += "<script>setInterval(()=>{fetch('/api/status').then(r=>r.json()).then(d=>{document.getElementById('doorState').textContent=d.doorState;document.getElementById('doorPosition').textContent=d.doorPosition;document.getElementById('limitUpper').textContent=d.limitUpper;document.getElementById('limitLower').textContent=d.limitLower;document.getElementById('buttonState').textContent=d.buttonState;document.getElementById('wifiRssi').textContent=d.wifiRssi;document.getElementById('uptime').textContent=d.uptime;document.getElementById('lastEvent').textContent=d.lastEvent;document.getElementById('alert').textContent=d.alert;});},2000);</script>";
    html += "</body></html>";
    return html;
}
