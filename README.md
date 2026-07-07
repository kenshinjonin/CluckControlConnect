🇬🇧 **English version**

# CluckControl Connect
*CluckControl Connect – Automation and peace of mind for your chicken coop!*

<p align="center">
  <img src="CluckControlConnect.png" alt="CluckControl Connect" width="200"/>
</p>

## Overview

CluckControl Connect is an open source home automation system for automated chicken coop management: automatic door opening/closing based on sunrise/sunset, manual control, multiple safety layers, remote supervision, Home Assistant integration, OTA updates...

## Main Features

- **State-based sun scheduling**: the door is continuously kept in the expected state (open by day, closed by night). A missed event (reboot, network outage) is automatically caught up — the door will not stay open all night because of a missed minute.
- **Manual override**: any manual command (push button, web interface, MQTT/Home Assistant) suspends the automation until the next day/night transition.
- **Safety layers**:
  - motor automatically stopped when a limit switch is reached;
  - maximum movement duration (`maxDoorMovementDuration`, 22 s by default) with `MOVEMENT_TIMEOUT` MQTT alert if exceeded (jammed door), without endless retries;
  - detection of the impossible "both limit switches active" state (`LIMIT_SWITCH_FAULT` alert, movements refused while the fault persists);
  - push button acts as an emergency stop while the door is moving;
  - task watchdog (`esp_task_wdt`, 120 s): automatic reboot if a critical task freezes.
- **Full MQTT integration**: states, commands, Last Will Testament (`poulailler/status` online/offline), Home Assistant Discovery for all entities (door cover with real position, limit switches, button, RSSI, uptime, last event, alerts).
- **Secured web interface**: HTTP Basic authentication, failed-attempt limiting with automatic expiry, protected OTA updates (ElegantOTA).
- **Resilience**: the local automation keeps working even if WiFi or the MQTT broker is unreachable (non-blocking reconnections).

## Required Hardware

- 1 × ESP32 (AZ-Delivery DevKit v4 or equivalent ESP32-WROOM-32)
- 1 × 12 V DC motor
- 1 × L298N H-Bridge module
- 2 × Limit switches (top and bottom, lever type)
- 1 × Push button (manual control)
- 1 × Suitable power supply for the ESP32 and the motor
- 1 × DS3231 RTC module (optional but recommended for time accuracy)
- 1 × 2.4 GHz WiFi access
- Suitable wiring (wires, terminals; twisted pair recommended for the limit switches)
- (Optional) 1 × Protective enclosure

### Wiring (default GPIO mapping)

| Function | GPIO | Note |
|---|---|---|
| Push button | 25 | internal pull-up, switch to GND |
| Motor Enable (EN) | 26 | |
| Motor IN1 | 27 | |
| Motor IN2 | 14 | |
| Upper limit switch | 32 | internal pull-up, switch to GND |
| Lower limit switch | 33 | internal pull-up, switch to GND |
| RTC DS3231 | 21 (SDA) / 22 (SCL) | default I²C bus |

> ⚠️ GPIO 6–11 are reserved for the internal SPI flash and must never be used. The mapping can be changed in `src/config.cpp`.

## Planned Hardware Evolutions

- **Obstruction detection by current sensing** (INA219 on the motor supply, or the current-sense output of a BTS7960 driver): a stalled motor while closing (a hen under the door!) would trigger a re-open and an alert, like a garage door.
- **Modern motor driver** (DRV8871, TB6612FNG or BTS7960) to replace the L298N: much lower voltage drop (the L298N wastes ~2 V), less heat, current sensing available.
- **PWM soft start/stop** (ESP32 `ledc`) to reduce mechanical stress on the door and the motor.
- **Supply/battery voltage monitoring** (resistor divider to an ADC pin, published over MQTT): power failures are the first cause of a stuck door.
- **Solar power** (panel + LiFePO4 battery + charge controller) for a fully autonomous coop.
- **Coop environment sensors** (BME280: temperature/humidity) and an optional light sensor as a plausibility cross-check of the computed sunrise/sunset.

## Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)

### Deployment
1. Clone this repository:
   ```sh
   git clone https://github.com/kenshinjonin/CluckControlConnect.git
   ```
2. Copy `include/secrets_example.h` to `include/secrets.h` and fill in your WiFi, MQTT and web credentials, your GPS coordinates and timezone/DST settings (never commit `secrets.h`!)
3. Build and upload the firmware to the ESP32 using PlatformIO
4. Access the web interface via the ESP32's IP address

## Usage

- Control and supervision via the secured web interface (live status, OPEN/CLOSE/STOP commands, link to OTA update page)
- Automatic integration into Home Assistant via MQTT Discovery
- MQTT commands on `poulailler/door/cmd`: `OPEN`, `CLOSE`, `STOP`
- OTA updates via `/update` (protected by the same credentials)

## Home Assistant: "door not closed after sunset" alert

Thanks to the Last Will Testament and the discovery based on the real door position, Home Assistant can reliably tell "closed" from "controller offline". Recommended safety automation:

1. Make sure the **MQTT integration** is configured in Home Assistant and points to the same broker as the ESP32.
2. After the first connection, the device **"Poulailler ESP32"** appears under *Settings → Devices & Services → MQTT*. Note the entity id of the door cover (typically `cover.porte_poulailler` — check it in *Developer Tools → States*).
3. Create a new automation (*Settings → Automations → Create → Edit in YAML*) and paste:

```yaml
alias: "Coop – door not closed after sunset"
description: "Alert if the door is not closed 45 min after sunset"
trigger:
  - platform: sun
    event: sunset
    offset: "00:45:00"
condition:
  - condition: not
    conditions:
      - condition: state
        entity_id: cover.porte_poulailler
        state: "closed"
action:
  - service: notify.mobile_app_my_phone   # adapt to your notify service
    data:
      title: "🐔 Chicken coop"
      message: "The coop door is NOT closed 45 minutes after sunset!"
mode: single
```

Note: the `not closed` condition also matches the `unavailable` state — you are alerted too if the ESP32 is down at that time. To be warned earlier about outages, add:

```yaml
alias: "Coop – controller offline"
trigger:
  - platform: state
    entity_id: cover.porte_poulailler
    to: "unavailable"
    for: "00:10:00"
action:
  - service: notify.mobile_app_my_phone
    data:
      message: "The coop controller has been offline for 10 minutes."
mode: single
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file.

## Author
Kenshinjonin – 2025


---
***************************************************************
---


🇫🇷 **Version française**

# CluckControl Connect
*CluckControl Connect – Automatisation et sérénité pour votre poulailler !*

<p align="center">
  <img src="CluckControlConnect.png" alt="CluckControl Connect" width="200"/>
</p>

## Présentation

CluckControl Connect est un système domotique open source pour la gestion automatisée d'un poulailler : ouverture/fermeture de porte selon le lever/coucher du soleil, contrôle manuel, sécurités multiples, supervision à distance, intégration Home Assistant, mises à jour OTA...

## Fonctionnalités principales

- **Asservissement jour/nuit basé sur l'état** : la porte est maintenue en continu dans l'état attendu (ouverte le jour, fermée la nuit). Un événement manqué (reboot, coupure réseau) est rattrapé automatiquement — la porte ne restera pas ouverte toute la nuit pour une minute ratée.
- **Priorité manuelle** : toute commande manuelle (bouton poussoir, interface web, MQTT/Home Assistant) suspend l'automatisme jusqu'à la prochaine transition jour/nuit.
- **Sécurités** :
  - arrêt automatique du moteur à l'atteinte d'une fin de course ;
  - durée maximale de mouvement (`maxDoorMovementDuration`, 22 s par défaut) avec alerte MQTT `MOVEMENT_TIMEOUT` en cas de dépassement (porte coincée), sans acharnement du moteur ;
  - détection de l'état impossible « deux fins de course actives » (alerte `LIMIT_SWITCH_FAULT`, mouvements refusés tant que la panne persiste) ;
  - le bouton poussoir sert d'arrêt d'urgence pendant un mouvement ;
  - watchdog de tâches (`esp_task_wdt`, 120 s) : reboot automatique si une tâche critique se fige.
- **Intégration MQTT complète** : états, commandes, Last Will Testament (`poulailler/status` online/offline), Home Assistant Discovery pour toutes les entités (cover de porte avec position réelle, fins de course, bouton, RSSI, uptime, dernier événement, alertes).
- **Interface web sécurisée** : authentification HTTP Basic, limitation des tentatives avec expiration automatique, mises à jour OTA protégées (ElegantOTA).
- **Résilience** : l'automatisme local continue de fonctionner même si le WiFi ou le broker MQTT est injoignable (reconnexions non bloquantes).

## Matériel nécessaire

- 1 × ESP32 (AZ-Delivery DevKit v4 ou équivalent ESP32-WROOM-32)
- 1 × Moteur DC 12 V
- 1 × Module pont en H L298N
- 2 × Capteurs de fin de course (haut et bas, type interrupteur à levier)
- 1 × Bouton poussoir (contrôle manuel)
- 1 × Alimentation adaptée pour l'ESP32 et le moteur
- 1 × Module RTC DS3231 (optionnel mais recommandé pour la précision horaire)
- 1 × Accès WiFi 2,4 GHz
- Câblage adapté (fils, borniers ; paire torsadée recommandée pour les fins de course)
- (Optionnel) 1 × Boîtier de protection

### Câblage (brochage par défaut)

| Fonction | GPIO | Remarque |
|---|---|---|
| Bouton poussoir | 25 | pull-up interne, contact vers GND |
| Enable moteur (EN) | 26 | |
| IN1 moteur | 27 | |
| IN2 moteur | 14 | |
| Fin de course haut | 32 | pull-up interne, contact vers GND |
| Fin de course bas | 33 | pull-up interne, contact vers GND |
| RTC DS3231 | 21 (SDA) / 22 (SCL) | bus I²C par défaut |

> ⚠️ Les GPIO 6 à 11 sont réservés à la flash SPI interne et ne doivent jamais être utilisés. Le brochage est modifiable dans `src/config.cpp`.

## Évolutions matérielles envisagées

- **Détection d'obstacle par mesure de courant** (INA219 sur l'alimentation moteur, ou la sortie de mesure d'un driver BTS7960) : un moteur qui cale à la fermeture (une poule sous la porte !) déclencherait une réouverture et une alerte, comme une porte de garage.
- **Driver moteur moderne** (DRV8871, TB6612FNG ou BTS7960) en remplacement du L298N : chute de tension bien plus faible (le L298N perd ~2 V), moins d'échauffement, mesure de courant disponible.
- **Démarrage/arrêt progressif en PWM** (`ledc` de l'ESP32) pour ménager la mécanique de la porte et le moteur.
- **Supervision de la tension d'alimentation** (pont diviseur vers une broche ADC, publiée sur MQTT) : les pannes d'alimentation sont la première cause de porte bloquée.
- **Alimentation solaire** (panneau + batterie LiFePO4 + régulateur de charge) pour un poulailler totalement autonome.
- **Capteurs d'ambiance** (BME280 : température/humidité) et éventuellement un capteur de luminosité en recoupement de plausibilité du calcul lever/coucher.

## Installation

### Prérequis
- [PlatformIO](https://platformio.org/) (extension VS Code ou CLI)

### Déploiement
1. Clonez ce dépôt :
   ```sh
   git clone https://github.com/kenshinjonin/CluckControlConnect.git
   ```
2. Copiez `include/secrets_example.h` vers `include/secrets.h` et renseignez vos identifiants WiFi, MQTT et web, vos coordonnées GPS et les paramètres de fuseau/heure d'été (ne jamais versionner `secrets.h` !)
3. Compilez et téléversez le firmware sur l'ESP32 via PlatformIO
4. Accédez à l'interface web via l'adresse IP de l'ESP32

## Utilisation

- Contrôle et supervision via l'interface web sécurisée (état en direct, commandes OUVRIR/FERMER/STOP, lien vers la page de mise à jour OTA)
- Intégration automatique dans Home Assistant via MQTT Discovery
- Commandes MQTT sur `poulailler/door/cmd` : `OPEN`, `CLOSE`, `STOP`
- Mises à jour OTA via `/update` (protégées par les mêmes identifiants)

## Home Assistant : alerte « porte non fermée après le coucher »

Grâce au Last Will Testament et à la discovery basée sur la position réelle de la porte, Home Assistant distingue de façon fiable « fermée » de « contrôleur hors ligne ». Automatisation de sécurité recommandée :

1. Vérifiez que l'**intégration MQTT** est configurée dans Home Assistant et pointe vers le même broker que l'ESP32.
2. Après la première connexion, l'appareil **« Poulailler ESP32 »** apparaît dans *Paramètres → Appareils et services → MQTT*. Notez l'identifiant d'entité du cover de porte (typiquement `cover.porte_poulailler` — vérifiez dans *Outils de développement → États*).
3. Créez une nouvelle automatisation (*Paramètres → Automatisations → Créer → Modifier en YAML*) et collez :

```yaml
alias: "Poulailler – porte non fermée après le coucher"
description: "Alerte si la porte n'est pas fermée 45 min après le coucher du soleil"
trigger:
  - platform: sun
    event: sunset
    offset: "00:45:00"
condition:
  - condition: not
    conditions:
      - condition: state
        entity_id: cover.porte_poulailler
        state: "closed"
action:
  - service: notify.mobile_app_mon_telephone   # à adapter à votre service de notification
    data:
      title: "🐔 Poulailler"
      message: "La porte du poulailler n'est PAS fermée 45 minutes après le coucher du soleil !"
mode: single
```

Remarque : la condition « non fermée » couvre aussi l'état `unavailable` — vous êtes donc alerté également si l'ESP32 est hors service à ce moment-là. Pour être prévenu plus tôt d'une panne, ajoutez :

```yaml
alias: "Poulailler – contrôleur hors ligne"
trigger:
  - platform: state
    entity_id: cover.porte_poulailler
    to: "unavailable"
    for: "00:10:00"
action:
  - service: notify.mobile_app_mon_telephone
    data:
      message: "Le contrôleur du poulailler est hors ligne depuis 10 minutes."
mode: single
```

## Licence

Ce projet est sous licence MIT. Voir le fichier [LICENSE](LICENSE).

## Auteur
Kenshinjonin – 2025

---
