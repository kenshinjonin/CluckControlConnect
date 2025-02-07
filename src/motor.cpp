#include <Arduino.h>
#include <RTClib.h>
#include <Timezone.h>

// Définition des broches utilisées
const uint8_t buttonPinNumber = 2;           // Broche du bouton poussoir
const uint8_t motorEnablePinNumber = 4;      // Broche d'activation du moteur (ENA sur L298N)
const uint8_t motorInput1PinNumber = 5;      // Broche d'entrée 1 du moteur (IN1 sur L298N)
const uint8_t motorInput2PinNumber = 6;      // Broche d'entrée 2 du moteur (IN2 sur L298N)
const uint8_t upperLimitSwitchPinNumber = 7; // Broche du capteur fin de course haut
const uint8_t lowerLimitSwitchPinNumber = 8; // Broche du capteur fin de course bas

// Variables d'état de la porte
bool isDoorMoving = false;
bool previousDoorDirectionUp = false;

// Constante pour définir le délai minimal entre deux appuis successifs sur le bouton (en millisecondes)
const unsigned long buttonDebounceDelay = 500;

// Variable pour suivre le moment du dernier appui sur le bouton
unsigned long lastButtonPressTimestamp = 0;

// Durée maximale de mouvement de la porte (en millisecondes)
const unsigned long maxDoorMovementDuration = 22000; // 22 secondes

// Définir les constantes pour les dates de changement d'heure d'été
const int DST_START_MONTH = 3; // Mars
const int DST_END_MONTH = 10;  // Octobre
const int DST_CHANGE_DAY = 25; // Après le 24ème jour

// Arrêter le mouvement de la porte
void stopPorte()
{
  isDoorMoving = false;
  digitalWrite(motorInput1PinNumber, LOW);
  digitalWrite(motorInput2PinNumber, LOW);
  digitalWrite(motorEnablePinNumber, LOW); // Désactiver le moteur
  Serial.println("Porte arrêtée");
}

// Ouvrir la porte
void ouvrirPorte()
{
  isDoorMoving = true;
  Serial.println("Ouverture de la porte");
  previousDoorDirectionUp = true;           // Mettre à jour la direction précédente
  digitalWrite(motorEnablePinNumber, HIGH); // Activer le moteur
  digitalWrite(motorInput1PinNumber, HIGH);
  digitalWrite(motorInput2PinNumber, LOW);

  unsigned long startTime = millis(); // Temps de départ pour suivre la durée de mouvement
  unsigned long elapsedTime = 0;      // Temps écoulé depuis le début du mouvement

  while (digitalRead(upperLimitSwitchPinNumber) == HIGH && elapsedTime < maxDoorMovementDuration)
  {
    // Vérifier si suffisamment de temps s'est écoulé pour laisser d'autres tâches s'exécuter (éviter un blocage)
    delay(1);                           // Attente d'une milliseconde
    elapsedTime = millis() - startTime; // Mettre à jour le temps écoulé
  }

  // Attendre 1 seconde
  delay(1000);
  stopPorte(); // Arrêter la porte
  Serial.println("Porte ouverte");
}

// Fermer la porte
void fermerPorte()
{
  isDoorMoving = true;
  Serial.println("Fermeture de la porte");
  previousDoorDirectionUp = false;          // Mettre à jour la direction précédente
  digitalWrite(motorEnablePinNumber, HIGH); // Activer le moteur
  digitalWrite(motorInput1PinNumber, LOW);
  digitalWrite(motorInput2PinNumber, HIGH);

  unsigned long startTime = millis(); // Temps de départ pour suivre la durée de mouvement
  unsigned long elapsedTime = 0;      // Temps écoulé depuis le début du mouvement

  while (digitalRead(lowerLimitSwitchPinNumber) == HIGH && elapsedTime < maxDoorMovementDuration)
  {
    // Vérifier si suffisamment de temps s'est écoulé pour laisser d'autres tâches s'exécuter (éviter un blocage)
    delay(1);                           // Attente d'une milliseconde
    elapsedTime = millis() - startTime; // Mettre à jour le temps écoulé
  }

  // Attendre 1 seconde
  delay(1000);
  stopPorte(); // Arrêter la porte
  Serial.println("Porte fermée");
}

// Fonction pour gérer l'interruption du bouton poussoir
void handleButtonPress()
{
  // Lire l'état du bouton poussoir
  int buttonState = digitalRead(buttonPinNumber);

  // Vérifier si le bouton est enfoncé
  if (buttonState == LOW && millis() - lastButtonPressTimestamp >= buttonDebounceDelay)
  {
    // Mettre à jour le moment du dernier appui sur le bouton
    lastButtonPressTimestamp = millis();
    Serial.println("Appui sur le bouton détecté");

    // Vérifier si la porte n'est pas en mouvement
    if (!isDoorMoving)
    {
      // Obtenir l'état des broches du moteur
      bool motorInput1State = digitalRead(motorInput1PinNumber);
      bool motorInput2State = digitalRead(motorInput2PinNumber);

      // Arrêter la porte si elle est en mouvement
      if (motorInput1State || motorInput2State)
      {
        stopPorte();
      }
      else
      {
        // Déterminer la direction en fonction des capteurs de fin de course
        bool upperLimitActive = digitalRead(upperLimitSwitchPinNumber) == LOW;
        bool lowerLimitActive = digitalRead(lowerLimitSwitchPinNumber) == LOW;

        if (upperLimitActive && lowerLimitActive)
        {
          // Les deux capteurs de fin de course sont inactifs, la porte est à mi-chemin
          if (previousDoorDirectionUp)
          {
            fermerPorte();
          }
          else
          {
            ouvrirPorte();
          }
        }
        else if (upperLimitActive)
        {
          fermerPorte();
        }
        else if (lowerLimitActive)
        {
          ouvrirPorte();
        }
      }
    }
  }
}

// Initialisation de la porte
void initPorte()
{
  // Déterminer si la porte doit être ouverte ou fermée au démarrage
  DateTime now = rtc.now();
  int currentMinuteOfDay = now.hour() * 60 + now.minute();

  if (currentMinuteOfDay >= sunriseTime && currentMinuteOfDay < sunsetTime)
  {
    // L'heure actuelle est entre le lever et le coucher du soleil, ouvrir la porte
    ouvrirPorte();
  }
  else
  {
    // L'heure actuelle est entre le coucher du soleil et le lever du lendemain, fermer la porte
    fermerPorte();
  }
}
