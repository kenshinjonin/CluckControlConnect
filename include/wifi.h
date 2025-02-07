#ifndef WIFI_H
#define WIFI_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Timezone.h>
#include <RTClib.h>

bool connectWiFi();
void printRSSI();
void WiFiReconnectTask(void *pvParameters);

#endif // WIFI_H
