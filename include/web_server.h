#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include <ElegantOTA.h>

class WebServerManager {
public:
    static void init();
    static void setupRoutes();
    static void handleClient();
    
private:
    static WebServer server;
    static String generateHtmlResponse();
}; 