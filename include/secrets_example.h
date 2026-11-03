/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * secrets_example.h
 * -----------------
 * Exemple de fichier de configuration des secrets et paramètres sensibles :
 *   - Identifiants WiFi, MQTT, Web
 *   - Coordonnées GPS du poulailler
 *
 * Le fuseau horaire et l'heure d'été se règlent dans src/config.cpp (timezone).
 *
 * À copier sous le nom secrets.h et à adapter à votre installation.
 * Ne pas versionner secrets.h !
 */

#pragma once

// --- Identifiants WiFi (exemple fictif) ---
#define SECRET_SSID     "MonReseauWiFi"
#define SECRET_PASSWORD "motdepasse_wifi"

// --- Identifiants MQTT et Web (exemple fictif) ---
#define SECRET_MQTT_SERVER "192.168.50.200"
#define SECRET_MQTT_USER   "user_demo"
#define SECRET_MQTT_PASS   "demo_pass_2025"
#define SECRET_WEB_USER    "superadmin"
#define SECRET_WEB_PASS    "supersecret"

// --- Coordonnées GPS du poulailler (exemple : Paris) ---
#define SECRET_LATITUDE 48.856614
#define SECRET_LONGITUDE 2.3522219
