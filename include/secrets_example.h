/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * secrets_example.h
 * -----------------
 * Exemple de fichier de configuration des secrets et paramètres sensibles :
 *   - Identifiants WiFi, MQTT, Web
 *   - Coordonnées GPS du poulailler
 *   - Fuseau horaire explicite
 *   - Paramètres DST (heure d'été, activable/désactivable)
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

// --- Fuseau horaire explicite (exemple : UTC+1 pour Paris en hiver) ---
#define SECRET_TIMEZONE_OFFSET 1 // Décalage horaire par rapport à UTC en hiver (ex : 1 pour UTC+1, la logique DST ajoutera +1 l'été)

// --- Paramètres DST (heure d'été) ---
#define SECRET_DST_ENABLED true           // true pour activer l'heure d'été, false pour la désactiver
#define SECRET_DST_START_MONTH 3          // Mois de début de l'heure d'été (mars)
#define SECRET_DST_END_MONTH 10           // Mois de fin de l'heure d'été (octobre)
