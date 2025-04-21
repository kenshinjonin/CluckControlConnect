// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include "web_server.h"
#include <ElegantOTA.h>
#include <WiFi.h>
#include <WebServer.h>
#include <base64.h>
#include "secrets.h"
#include <Arduino.h>
#include "mqtt_manager.h"
#include "door_controller.h"
#include "time_manager.h"

namespace {
const char* www_username = SECRET_WEB_USER;
const char* www_password = SECRET_WEB_PASS;
const int MAX_LOGIN_ATTEMPTS = 5;
int loginAttempts = 0;

bool isAuthenticated(WebServer& server) {
    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        server.send(403, "text/plain", "Trop de tentatives. Accès bloqué.");
        return false;
    }
    if (!server.hasHeader("Authorization")) {
        server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
        server.send(401, "text/plain", "Authentification requise");
        loginAttempts++;
        return false;
    }
    String auth = server.header("Authorization");
    auth.replace("Basic ", "");
    String expectedAuth = base64::encode(String(www_username) + ":" + www_password);
    if (auth == expectedAuth) {
        loginAttempts = 0;
        return true;
    } else {
        server.sendHeader("WWW-Authenticate", "Basic realm=\"Secure Area\"");
        server.send(401, "text/plain", "Authentification requise");
        loginAttempts++;
        return false;
    }
}
}

WebServer WebServerManager::server(80);

void WebServerManager::init() {
    server.begin();
    setupRoutes();
    ElegantOTA.begin(&server);
    server.enableCORS(true);
}

void WebServerManager::handleClient() {
    server.handleClient();
}

void WebServerManager::setupRoutes() {
    server.on("/", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        server.send(200, "text/html", generateHtmlResponse());
    });
    
    server.on("/update", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        server.send(200, "text/html", generateHtmlResponse());
    });

    server.on("/api/status", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        String json = "{";
        // État de la porte
        String doorState = DoorController::getIsDoorMoving() ? "MOVING" : "STOPPED";
        // Position
        int upper = digitalRead(upperLimitSwitchPinNumber);
        int lower = digitalRead(lowerLimitSwitchPinNumber);
        String doorPosition = upper == LOW ? "OPEN" : (lower == LOW ? "CLOSED" : "UNKNOWN");
        // Fins de course
        String limitUpper = upper == LOW ? "ON" : "OFF";
        String limitLower = lower == LOW ? "ON" : "OFF";
        // Bouton poussoir
        int buttonState = digitalRead(buttonPinNumber);
        String button = buttonState == LOW ? "ON" : "OFF";
        // Dernier événement (à améliorer si besoin)
        String lastEvent = "-";
        // Alerte (placeholder)
        String alert = "-";
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
            if (action == "OPEN") DoorController::open();
            else if (action == "CLOSE") DoorController::close();
            else if (action == "STOP") DoorController::stop();
        }
        server.sendHeader("Location", "/");
        server.send(303);
    });
}

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
    html += "</section>";
    html += "<footer>CluckControl Connect © 2025</footer>";
    html += "<script>setInterval(()=>{fetch('/api/status').then(r=>r.json()).then(d=>{document.getElementById('doorState').textContent=d.doorState;document.getElementById('doorPosition').textContent=d.doorPosition;document.getElementById('limitUpper').textContent=d.limitUpper;document.getElementById('limitLower').textContent=d.limitLower;document.getElementById('buttonState').textContent=d.buttonState;document.getElementById('wifiRssi').textContent=d.wifiRssi;document.getElementById('uptime').textContent=d.uptime;document.getElementById('lastEvent').textContent=d.lastEvent;document.getElementById('alert').textContent=d.alert;});},2000);</script>";
    html += "</body></html>";
    return html;
}