#include "web_server.h"
#include <ElegantOTA.h>
#include <WiFi.h>
#include <WebServer.h>

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
        server.send(200, "text/html", generateHtmlResponse());
    });
    
    server.on("/update", HTTP_GET, []() {
        server.send(200, "text/html", generateHtmlResponse());
    });
}

String WebServerManager::generateHtmlResponse() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Contrôle de la Porte</title></head><body>";
    html += "<h1>Contrôle de la Porte</h1>";
    // Ajoutez le contenu HTML selon vos besoins
    html += "</body></html>";
    return html;
}