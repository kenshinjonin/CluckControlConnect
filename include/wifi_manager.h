// CluckControl Connect © 2025 - Auteur : Kenshinjonin
// Licence : MIT

#include <WiFi.h>
#include "config.h"

class WiFiManager {
public:
    static bool connect();
    static void printRSSI();
    static void reconnectTask(void *pvParameters);
private:
    static WiFiClient wifiClient;
};