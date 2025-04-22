🇬🇧 **English version**

# CluckControl Connect
*CluckControl Connect – Automation and peace of mind for your chicken coop!*

<p align="center">
  <img src="CluckControlConnect.png" alt="CluckControl Connect" width="200"/>
</p>

## Overview

CluckControl Connect is an open source home automation system for automated chicken coop management: automatic door opening/closing based on sunrise/sunset, manual control, safety, remote supervision, Home Assistant integration, OTA updates...

## Required Hardware
- 1 × ESP32
- 1 × 12v DC Motor
- 1 × L298N H-Bridge module
- 2 × Limit switches (top and bottom, lever type)
- 1 × Push button (for manual control)
- 1 × Suitable power supply for ESP32 and the motor
- 1 × DS3231 RTC module (optional but recommended for time accuracy)
- 1 × 2.4 GHz WiFi access
- 1 × Suitable wiring (wires, terminals, etc.)
- (Optional) 1 × Protective enclosure

## Main Features
- Automatic sunrise/sunset calculation
- Manual control via push button or secure web interface
- Full MQTT integration (states, commands, Home Assistant Discovery)
- OTA updates

## Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)

### Deployment
1. Clone this repository:
   ```sh
   git clone https://github.com/kenshinjonin/Poulailler_V2.git
   ```
2. Add your WiFi, MQTT, web credentials and GPS coordinates in `include/secrets.h` (see example provided, do not version this file!)
3. Build and upload the firmware to the ESP32 using PlatformIO
4. Access the web interface via the ESP32's IP address

## Usage
- Control and supervision via the secure web interface
- Automatic integration into Home Assistant via MQTT Discovery
- MQTT commands: open, close, stop, supervision
- OTA updates via the web interface

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file.

## Author
Kenshinjonin – 2025

---
<hr style="border: 2px solid #444; margin: 32px 0;">
---

🇫🇷 **Version française**

# CluckControl Connect
*CluckControl Connect – Automatisation et sérénité pour votre poulailler !*

<p align="center">
  <img src="CluckControlConnect.png" alt="CluckControl Connect" width="200"/>
</p>

## Présentation

CluckControl Connect est un système domotique open source pour la gestion automatisée d’un poulailler : ouverture/fermeture de porte selon le lever/coucher du soleil, contrôle manuel, sécurité, supervision à distance, intégration Home Assistant, mises à jour OTA...

## Matériel nécessaire
- 1 × ESP32
- 1 × Moteur DC 12v
- 1 module H-Bridge L298N
- 2 × Capteurs de fin de course (haut et bas, type interrupteur à levier)
- 1 × Bouton poussoir (pour contrôle manuel)
- 1 × Alimentation adaptée pour l’ESP32 et le moteur
- 1 × Module RTC DS3231 (optionnel mais recommandé pour la précision horaire)
- 1 × Accès WiFi 2,4 GHz
- 1 × Câblage adapté (fils, borniers, etc.)
- (Optionnel) 1 × Boîtier de protection

## Fonctionnalités principales
- Calcul automatique lever/coucher du soleil
- Contrôle manuel via bouton poussoir ou interface web sécurisée
- Intégration MQTT complète (états, commandes, Home Assistant Discovery)
- Mises à jour OTA

## Installation

### Prérequis
- [PlatformIO](https://platformio.org/) (extension VS Code ou CLI)

### Déploiement
1. Clonez ce dépôt :
   ```sh
   git clone https://github.com/kenshinjonin/Poulailler_V2.git
   ```
2. Ajoutez vos identifiants WiFi, MQTT, web et vos coordonnées GPS dans `include/secrets.h` (voir exemple fourni, ne pas versionner ce fichier !)
3. Compilez et téléversez le firmware sur l’ESP32 via PlatformIO
4. Accédez à l’interface web via l’adresse IP de l’ESP32

## Utilisation
- Contrôle et supervision via l’interface web sécurisée
- Intégration automatique dans Home Assistant via MQTT Discovery
- Commandes MQTT : ouvrir, fermer, stop, supervision
- Mises à jour OTA via l’interface web

## Licence

Ce projet est sous licence MIT. Voir le fichier [LICENSE](LICENSE).

## Auteur
Kenshinjonin – 2025

---