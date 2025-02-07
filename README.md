# Poulailler_V2

Code ESP32 pour automatiser une porte de poulailler avec calcul automatique de la position du soleil, MQTT et mise à jour OTA.

(Bientôt) Pleinement fonctionnel avec HomeAssistant mais devrait fonctionner avec n'importe quel système gérant MQTT.

## Modularity and Readability Improvements

The code has been refactored to improve modularity and readability. The main logic has been separated into different files, each handling a specific concern:

- `wifi.cpp`: Handles WiFi connection and RSSI printing.
- `rtc.cpp`: Manages RTC time updates and sunrise/sunset calculations.
- `motor.cpp`: Controls the motor for opening and closing the door.
- `web_server.cpp`: Manages the web server and its endpoints.

This separation of concerns makes the code easier to understand, maintain, and extend.

TODO :
- MQTT

=================================================================



Esp32 code, to automate a chicken coop door using automatic sun position calculation, MQTT, and OTA update.

(soon) Fully tested with HomeAssistant but should work with any MQTT-aware system.

## Modularity and Readability Improvements

The code has been refactored to improve modularity and readability. The main logic has been separated into different files, each handling a specific concern:

- `wifi.cpp`: Handles WiFi connection and RSSI printing.
- `rtc.cpp`: Manages RTC time updates and sunrise/sunset calculations.
- `motor.cpp`: Controls the motor for opening and closing the door.
- `web_server.cpp`: Manages the web server and its endpoints.

This separation of concerns makes the code easier to understand, maintain, and extend.

TODO :
- MQTT
