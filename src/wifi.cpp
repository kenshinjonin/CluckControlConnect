#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Timezone.h>
#include <RTClib.h>

// Paramètres de connexion WiFi
const char *ssid = "SSID";
const char *password = "PASSWORD";

// Objets pour se connecter au réseau WiFi
WiFiClient wifiClient;
WiFiUDP ntpUDP;

// Objet NTPClient pour récupérer l'heure via NTP
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org");

// Créer un objet RTC_DS3231 pour interagir avec le module RTC
RTC_DS3231 rtc;

// Définir le fuseau horaire (ajuster en fonction de votre emplacement géographique)
TimeChangeRule myDST = {"CEST", Last, Sun, Mar, 2, 120}; // Heure d'été (Central European Summer Time)
TimeChangeRule mySTD = {"CET", Last, Sun, Oct, 3, 60};   // Heure d'hiver (Central European Time)
Timezone myTZ(myDST, mySTD);                             // Créer un objet Timezone avec les règles de changement d'heure

// Connexion au réseau WiFi
bool connectWiFi()
{
  unsigned long startTime = millis();
  unsigned long timeout = 30000; // Timeout de 30 secondes
  bool connected = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Tentative de connexion WiFi...");

  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout)
  {
    delay(1000);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    connected = true;
    Serial.println("");
    Serial.println("WiFi connecté avec succès à : ");
    Serial.println(ssid);
    Serial.print("Adresse IP : ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI : ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  else
  {
    Serial.println("");
    Serial.println("Échec de la connexion WiFi. Vérifiez vos identifiants ou votre réseau.");
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
  }

  return connected;
}

// Fonction pour obtenir et afficher le RSSI en dBm
void printRSSI()
{
  long rssi = WiFi.RSSI();
  Serial.print("RSSI: ");
  Serial.print(rssi);
  Serial.println(" dBm");
}

// Tâche pour vérifier la connexion WIFI toutes les 15 minutes
TaskHandle_t WiFiReconnectTaskHandle = NULL; // Variable pour stocker le handle de la tâche WiFiReconnectTask
void WiFiReconnectTask(void *pvParameters)
{
  for (;;)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Reconnexion WiFi...");
      connectWiFi();
    }
    vTaskDelay(pdMS_TO_TICKS(15 * 60 * 1000)); // Attendre 15 minutes
  }
}
