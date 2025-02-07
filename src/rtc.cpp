#include <RTClib.h>
#include <Timezone.h>
#include <Dusk2Dawn.h>

// Créer un objet RTC_DS3231 pour interagir avec le module RTC
RTC_DS3231 rtc;

// Définir le fuseau horaire (ajuster en fonction de votre emplacement géographique)
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120}; // Heure d'été (Central European Summer Time)
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};   // Heure d'hiver (Central European Time)
Timezone myTZ(myDST, mySTD);                             // Créer un objet Timezone avec les règles de changement d'heure

// Paramètres de l'emplacement géographique (à ajuster en fonction de votre emplacement)
const double latitude = 46.308352907180954; // Latitude en degrés décimaux
const double longitude = 4.887889949495334; // Longitude en degrés décimaux
int UTC_OFFSET = 1;

// Variables pour stocker l'heure du lever et du coucher du soleil
int sunriseTime = 0; // Heure du lever du soleil (en minutes depuis minuit)
int sunsetTime = 0;  // Heure du coucher du soleil (en minutes depuis minuit)
char sunriseTimeFormatted[6];
char sunsetTimeFormatted[6];

// Mettre à jour l'heure du module DS3231 via NTP et gérer le passage automatique à l'heure d'été/heure d'hiver
bool updateDS3231Time()
{
  if (!timeClient.update())
  {               // Récupérer l'heure NTP et vérifier si la mise à jour a réussi
    return false; // La mise à jour NTP a échoué, donc on ne met pas à jour l'heure du DS3231
  }

  time_t utcTime = timeClient.getEpochTime(); // Obtenir l'heure UTC en secondes depuis le 1er janvier 1970
  if (utcTime == 0)
  {
    return false; // La récupération de l'heure UTC a échoué
  }

  time_t localTime = myTZ.toLocal(utcTime); // Convertir l'heure UTC en heure locale
  if (localTime == 0)
  {
    return false; // La conversion de l'heure locale a échoué
  }

  rtc.adjust(DateTime(localTime)); // Mettre à jour l'heure du module RTC avec l'heure locale
  return true;                     // L'heure a été mise à jour avec succès
}

// Fonction pour formater le temps en "HHhMM" sans utiliser strftime
void formatTime(int time, char *buffer)
{
  int hours = time / 60;
  int minutes = time % 60;
  sprintf(buffer, "%02dh%02d", hours, minutes);
}

// Calculer l'heure du lever du soleil et du coucher du soleil
void calculateSunriseSunset()
{
  DateTime now = rtc.now();
  int year = now.year();
  int month = now.month();
  int day = now.day();
  int dayOfWeek = now.dayOfTheWeek();

  // Vérifier si nous sommes dans la période de l'heure d'été (DST)
  if ((month > DST_START_MONTH && month < DST_END_MONTH) ||
      (month == DST_START_MONTH && (dayOfWeek == 1 && day >= DST_CHANGE_DAY)) ||
      (month == DST_END_MONTH && (dayOfWeek == 1 && day < DST_CHANGE_DAY)))
  {
    UTC_OFFSET = 2; // Heure d'été
  }
  else
  {
    UTC_OFFSET = 1; // Heure standard
  }

  // Créer l'objet Dusk2Dawn une seule fois si possible
  static Dusk2Dawn myLocation(latitude, longitude, UTC_OFFSET * 60);

  // Calculer le lever et le coucher du soleil
  int sunriseTime = myLocation.sunrise(year, month, day, false);
  int sunsetTime = myLocation.sunset(year, month, day, false);

  // Convertir les heures locales en "HHhMM"
  formatTime(sunriseTime, sunriseTimeFormatted);
  formatTime(sunsetTime, sunsetTimeFormatted);

  // Afficher dans la console série
  Serial.print("Heure du lever du soleil : ");
  Serial.println(sunriseTimeFormatted);
  Serial.print("Heure du coucher du soleil : ");
  Serial.println(sunsetTimeFormatted);
}

// Tâche pour mettre à jour l'heure toutes les 24 heures
TaskHandle_t updateTimeTaskHandle = NULL; // Variable pour stocker le handle de la tâche updateTimeTask
void updateTimeTask(void *parameter)
{
  for (;;)
  {
    vTaskDelay(pdMS_TO_TICKS(86400000)); // Attendre 24 heures (en millisecondes)
    updateDS3231Time();                  // Mettre à jour l'heure du module DS3231
    Serial.println("Mise à jour de l'heure");
  }
}

// Tâche pour recalculer l'heure du lever du soleil et du coucher du soleil tous les jours à 00h01
TaskHandle_t calculateSunriseSunsetTaskHandle = NULL; // Variable pour stocker le handle de la tâche calculateSunriseSunsetTask
void calculateSunriseSunsetTask(void *parameter)
{
  const int secondsPerDay = 86400; // Nombre de secondes dans une journée
  const int targetHour = 0;        // Heure cible (00h)
  const int targetMinute = 1;      // Minute cible (01)

  for (;;)
  {
    DateTime now = rtc.now();
    DateTime nextExecution(now.year(), now.month(), now.day(), targetHour, targetMinute, 0);

    // Si l'heure actuelle est après 00h01, définir la prochaine exécution pour le jour suivant
    if (now >= nextExecution)
    {
      nextExecution = nextExecution + TimeSpan(1, 0, 0, 0);
    }

    // Calculer le délai en secondes jusqu'à la prochaine exécution
    long delaySeconds = (nextExecution.unixtime() - now.unixtime());

    // Convertir le délai en millisecondes et attendre
    vTaskDelay(pdMS_TO_TICKS(delaySeconds * 1000));

    calculateSunriseSunset(); // Recalculer l'heure du lever et du coucher du soleil
  }
}
