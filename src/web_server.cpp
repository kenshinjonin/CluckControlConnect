/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * web_server.cpp
 * --------------
 * Gère l'interface web embarquée du système :
 *   - Authentification HTTP basique avec limitation des tentatives
 *   - Protection anti-CSRF des commandes de la porte
 *   - Définition des routes (page principale, API JSON, commandes)
 *   - Intégration ElegantOTA (protégée par la même authentification)
 *   - Publication d'informations système (état, position, RSSI, version, etc.)
 *
 * La page HTML est servie depuis la mémoire programme et le JSON est construit
 * dans un tampon fixe : aucune allocation dynamique n'a lieu par requête, ce qui
 * évite de fragmenter le tas au fil des mois de fonctionnement (l'interface est
 * interrogée toutes les deux secondes par chaque onglet ouvert).
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
const char *www_username = SECRET_WEB_USER;
const char *www_password = SECRET_WEB_PASS;
const int MAX_LOGIN_ATTEMPTS = 5;
const unsigned long LOGIN_ATTEMPTS_RESET_MS = 5 * 60 * 1000; // Expiration du compteur d'échecs
int loginAttempts = 0;
unsigned long lastFailedAttemptTimestamp = 0;

// En-tête que seule notre propre page peut poser
const char *CSRF_HEADER = "X-Coop-Request";

// --- Vérification de l'authentification HTTP basique ---
bool isAuthenticated(WebServer &server) {
    // Les identifiants valides passent en toutes circonstances. Compter les
    // échecs avant de vérifier le mot de passe permettrait à n'importe qui sur
    // le réseau local de verrouiller l'utilisateur légitime en cinq requêtes.
    if (server.authenticate(www_username, www_password)) {
        loginAttempts = 0;
        return true;
    }

    // Le compteur d'échecs expire : pas de verrouillage définitif jusqu'au reboot
    if (loginAttempts > 0 && millis() - lastFailedAttemptTimestamp > LOGIN_ATTEMPTS_RESET_MS) {
        loginAttempts = 0;
    }
    // Ne compter que les requêtes portant des identifiants (mauvais mot de passe) :
    // la première requête d'un navigateur n'a pas encore de header Authorization
    if (server.hasHeader("Authorization")) {
        loginAttempts++;
        lastFailedAttemptTimestamp = millis();
    }

    if (loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        server.send(429, "text/plain", "Trop de tentatives. Reessayez dans quelques minutes.");
        return false;
    }

    server.requestAuthentication();
    return false;
}

// --- Protection anti-CSRF des commandes ---
// Les identifiants HTTP Basic sont renvoyés automatiquement par le navigateur,
// y compris sur un POST de formulaire déclenché par un site tiers : sans
// contrôle, une page malveillante visitée par l'utilisateur pourrait ouvrir la
// porte. L'en-tête personnalisé impose un preflight CORS, auquel on ne répond
// pas. Le contrôle de l'en-tête Host bloque en complément le DNS rebinding, qui
// permettrait sinon à l'attaquant de se présenter comme la même origine.
bool isTrustedRequest(WebServer &server) {
    if (!server.hasHeader(CSRF_HEADER)) return false;

    String host = server.hostHeader();
    int colon = host.indexOf(':');
    if (colon >= 0) host = host.substring(0, colon);

    IPAddress parsed;
    return parsed.fromString(host); // N'accepte qu'une adresse IP littérale
}

// --- Page principale, servie depuis la mémoire programme ---
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='fr'><head>
<meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>
<title>Poulailler - Controle de la Porte</title>
<style>
body{font-family:sans-serif;background:#f4f4f4;margin:0;padding:0;color:#222}
h1{background:#4CAF50;color:#fff;padding:12px;margin:0;font-size:1.3em}
section{margin:16px}
button{padding:10px 20px;margin:5px;font-size:16px;cursor:pointer}
div.status{margin:8px 0;padding:10px;background:#fff;border-radius:5px;box-shadow:0 2px 4px #ccc}
footer{margin:20px;text-align:center;color:#888;font-size:.85em}
.offline{color:#c00;font-weight:bold}
</style></head><body>
<h1>Poulailler - Controle de la Porte</h1>
<section>
<div class='status'><b>Etat de la porte :</b> <span id='doorState'>?</span></div>
<div class='status'><b>Position :</b> <span id='doorPosition'>?</span></div>
<div class='status'><b>Fin de course haut :</b> <span id='limitUpper'>?</span> | <b>Fin de course bas :</b> <span id='limitLower'>?</span></div>
<div class='status'><b>Bouton poussoir :</b> <span id='buttonState'>?</span></div>
<div class='status'><b>Ouverture :</b> <span id='sunrise'>?</span> (<span id='sunriseOffset'>?</span> min) | <b>Fermeture :</b> <span id='sunset'>?</span> (<span id='sunsetOffset'>?</span> min)</div>
<div class='status'><b>MQTT :</b> <span id='mqtt'>?</span> | <b>RSSI WiFi :</b> <span id='wifiRssi'>?</span> dBm</div>
<div class='status'><b>Uptime :</b> <span id='uptime'>?</span> s</div>
<div class='status'><b>Dernier evenement :</b> <span id='lastEvent'>?</span></div>
<div class='status'><b>Alerte :</b> <span id='alert'>?</span></div>
<div>
<button onclick="cmd('OPEN')">Ouvrir</button>
<button onclick="cmd('CLOSE')">Fermer</button>
<button onclick="cmd('STOP')">Stop</button>
</div>
<p><a href='/update'>Mise a jour OTA</a></p>
</section>
<footer>CluckControl Connect &copy; 2025 - firmware <span id='firmware'>?</span></footer>
<script>
function cmd(a){
  fetch('/cmd',{method:'POST',
    headers:{'X-Coop-Request':'1','Content-Type':'application/x-www-form-urlencoded'},
    body:'action='+a}).then(refresh);
}
function set(id,v){document.getElementById(id).textContent=v;}
function refresh(){
  fetch('/api/status').then(r=>r.json()).then(d=>{
    for(const k in d){if(document.getElementById(k))set(k,d[k]);}
    document.getElementById('doorState').classList.remove('offline');
  }).catch(()=>{
    // Sans ce marquage, un controleur hors ligne afficherait indefiniment
    // les dernieres valeurs connues et passerait pour fonctionnel.
    set('doorState','HORS LIGNE');
    document.getElementById('doorState').classList.add('offline');
  });
}
setInterval(refresh,2000); refresh();
</script>
</body></html>
)rawliteral";
} // namespace

WebServer WebServerManager::server(80);

// --- Initialisation du serveur web et des routes ---
void WebServerManager::init() {
    // Nécessaire pour que hasHeader() fonctionne sur ces en-têtes
    const char *headerKeys[] = {"Authorization", CSRF_HEADER};
    server.collectHeaders(headerKeys, 2);

    setupRoutes();
    ElegantOTA.begin(&server);
    ElegantOTA.setAuth(www_username, www_password); // Protège /update (UI et upload OTA)
    server.begin();
}

// --- Tâche FreeRTOS : traitement des requêtes web et OTA ---
// Volontairement hors watchdog : un téléversement OTA est traité dans un unique
// appel à handleClient() et peut dépasser plusieurs dizaines de secondes sur une
// liaison faible. Un redémarrage en plein flashage laisserait un firmware
// incomplet, alors qu'un serveur web figé n'affecte en rien la sécurité de la
// porte, dont la boucle de contrôle est indépendante.
void WebServerManager::webServerTask(void *parameter) {
    for (;;) {
        server.handleClient();
        ElegantOTA.loop(); // Requis par ElegantOTA v3 (finalise le reboot post-mise à jour)
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

// --- Définition des routes HTTP ---
void WebServerManager::setupRoutes() {
    server.on("/", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;
        server.send_P(200, "text/html", INDEX_HTML);
    });

    // Note : /update est servi par ElegantOTA (protégé via ElegantOTA.setAuth)

    server.on("/api/status", HTTP_GET, []() {
        if (!isAuthenticated(server)) return;

        int upper = digitalRead(config.upperLimitSwitchPinNumber);
        int lower = digitalRead(config.lowerLimitSwitchPinNumber);
        int buttonState = digitalRead(config.buttonPinNumber);

        char sunrise[6], sunset[6];
        TimeManager::formatTime(TimeManager::getSunriseMinutes(), sunrise, sizeof(sunrise));
        TimeManager::formatTime(TimeManager::getSunsetMinutes(), sunset, sizeof(sunset));

        char json[512];
        snprintf(json, sizeof(json),
                 "{\"doorState\":\"%s\",\"doorPosition\":\"%s\",\"limitUpper\":\"%s\","
                 "\"limitLower\":\"%s\",\"buttonState\":\"%s\",\"wifiRssi\":\"%d\","
                 "\"uptime\":\"%lu\",\"lastEvent\":\"%s\",\"alert\":\"%s\","
                 "\"firmware\":\"%s\",\"mqtt\":\"%s\",\"sunrise\":\"%s\",\"sunset\":\"%s\","
                 "\"sunriseOffset\":\"%+d\",\"sunsetOffset\":\"%+d\"}",
                 DoorController::getIsDoorMoving() ? "MOVING" : "STOPPED",
                 DoorController::getDoorPosition(),
                 upper == LOW ? "ON" : "OFF",
                 lower == LOW ? "ON" : "OFF",
                 buttonState == LOW ? "ON" : "OFF",
                 WiFi.RSSI(),
                 millis() / 1000,
                 DoorController::getLastEvent(),
                 DoorController::getLastAlert(),
                 FIRMWARE_VERSION,
                 MQTTManager::isConnected() ? "connecte" : "deconnecte",
                 sunrise, sunset,
                 TimeManager::getSunriseOffset(),
                 TimeManager::getSunsetOffset());

        server.send(200, "application/json", json);
    });

    server.on("/cmd", HTTP_POST, []() {
        if (!isAuthenticated(server)) return;
        if (!isTrustedRequest(server)) {
            server.send(403, "text/plain", "Requete refusee");
            return;
        }
        if (server.hasArg("action")) {
            String action = server.arg("action");
            if (action == "OPEN") DoorController::requestCommand(DoorCommand::Open, true);
            else if (action == "CLOSE") DoorController::requestCommand(DoorCommand::Close, true);
            else if (action == "STOP") DoorController::requestCommand(DoorCommand::Stop, true);
        }
        server.send(204);
    });
}
