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

## Schéma de câblage
*(À compléter avec un schéma ou une photo)*

## Licence

Ce projet est sous licence MIT. Voir le fichier [LICENSE](LICENSE).

## Auteur
Kenshinjonin – 2025

---

*CluckControl Connect – Automatisation et sérénité pour votre poulailler !*