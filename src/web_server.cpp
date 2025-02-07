#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Timezone.h>
#include "motor.h"

// Serveur WEB
AsyncWebServer server(80);

// Variables externes
extern RTC_DS3231 rtc;
extern int sunriseTime;
extern int sunsetTime;
extern char sunriseTimeFormatted[6];
extern char sunsetTimeFormatted[6];
extern bool isDoorMoving;
extern const uint8_t upperLimitSwitchPinNumber;
extern const uint8_t lowerLimitSwitchPinNumber;
extern const char *ssid;
extern const char *password;

// Fonction pour formater le temps en "HHhMM" sans utiliser strftime
void formatTime(int time, char *buffer)
{
  int hours = time / 60;
  int minutes = time % 60;
  sprintf(buffer, "%02dh%02d", hours, minutes);
}

// Initialisation du serveur web
void initWebServer()
{
  // Page WEB
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Obtenez les informations nécessaires
    char rtcTimeFormatted[20];
    DateTime now = rtc.now();
    int currentMinuteOfDay = now.hour() * 60 + now.minute();
    formatTime(currentMinuteOfDay, rtcTimeFormatted);

    String wifiSignalStrength = String(WiFi.RSSI());
    String sunriseTimeStr = String(sunriseTimeFormatted);
    String sunsetTimeStr = String(sunsetTimeFormatted);
    String upperLimitSwitchState = (digitalRead(upperLimitSwitchPinNumber) == HIGH) ? "Fermé" : "Ouvert";
    String lowerLimitSwitchState = (digitalRead(lowerLimitSwitchPinNumber) == HIGH) ? "Fermé" : "Ouvert";
    String doorMovementState = (isDoorMoving) ? "En mouvement" : "Arrêté";

    // Construisez la réponse HTML
    String html = "<html><head><style>h1 {text-align:center;font-weight:bold;}</style></head><body>";
    html += "<h1>POULAILLER</h1>";
    html += "<p>Heure actuelle (RTC) : " + String(rtcTimeFormatted) + "</p>";
    html += "<p>Force du signal WiFi : " + wifiSignalStrength + " dBm</p>";
    html += "<p>Heure lever du soleil : " + sunriseTimeStr + "</p>";
    html += "<p>Heure coucher du soleil : " + sunsetTimeStr + "</p>";
    html += "<p>Capteur haut (porte) : " + upperLimitSwitchState + "</p>";
    html += "<p>Capteur bas (porte) : " + lowerLimitSwitchState + "</p>";
    html += "<p>État de la porte : " + doorMovementState + "</p>";
    html += "<button onclick=\"fermerPorte()\">FERMER</button>";
    html += "<button onclick=\"stopPorte()\">STOP</button>";
    html += "<button onclick=\"ouvrirPorte()\">OUVRIR</button>";
    html += "</body><script>";
    html += "function fermerPorte() { fetch('/fermer'); }";
    html += "function stopPorte() { fetch('/stop'); }";
    html += "function ouvrirPorte() { fetch('/ouvrir'); }";
    html += "</script></html>";

    // Envoyez la réponse au client
    request->send(200, "text/html", html); });

  // Endpoint pour fermer la porte
  server.on("/fermer", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Appeler la fonction pour fermer la porte
    fermerPorte();

    // Envoyer une réponse vide au client
    request->send(200, "text/plain", ""); });

  // Endpoint pour arrêter la porte
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Appeler la fonction pour arrêter la porte
    stopPorte();

    // Envoyer une réponse vide au client
    request->send(200, "text/plain", ""); });

  // Endpoint pour ouvrir la porte
  server.on("/ouvrir", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // Appeler la fonction pour ouvrir la porte
    ouvrirPorte();

    // Envoyer une réponse vide au client
    request->send(200, "text/plain", ""); });

  // Start ElegantOTA
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");
}
