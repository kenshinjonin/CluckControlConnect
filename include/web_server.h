#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <RTClib.h>
#include <WiFi.h>
#include <Timezone.h>
#include "motor.h"

void initWebServer();

#endif // WEB_SERVER_H
