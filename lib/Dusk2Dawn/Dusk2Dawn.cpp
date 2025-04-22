#include "Dusk2Dawn.h"

Dusk2Dawn::Dusk2Dawn(float latitude, float longitude, float timezone) {
  _latitude = latitude;
  _longitude = longitude;
  _timezone = timezone;
}

int Dusk2Dawn::sunrise(int year, int month, int day, bool isDST) {
  return sunriseSet(true, year, month, day, isDST);
}

int Dusk2Dawn::sunset(int year, int month, int day, bool isDST) {
  return sunriseSet(false, year, month, day, isDST);
}

int Dusk2Dawn::sunriseSet(bool sunrise, int year, int month, int day, bool isDST) {
  float jday = jDay(year, month, day);
  float riseSetUTC = sunriseSetUTC(sunrise, jday, _latitude, _longitude);
  float minutes = riseSetUTC + (_timezone * 60);
  if (isDST) minutes += 60;
  return static_cast<int>(minutes);
}

float Dusk2Dawn::sunriseSetUTC(bool sunrise, float jday, float latitude, float longitude) {
  float t = fractionOfCentury(jday);
  float eqTime = equationOfTime(t);
  float solarDec = sunDeclination(t);
  float hourAngle = hourAngleSunrise(latitude, solarDec);
  if (!sunrise) hourAngle = -hourAngle;
  float delta = longitude + radToDeg(hourAngle);
  float timeUTC = 720 - (4.0 * delta) - eqTime;
  return timeUTC;
}

float Dusk2Dawn::jDay(int year, int month, int day) {
  if (month <= 2) {
    year -= 1;
    month += 12;
  }
  int A = floor(year / 100.0);
  int B = 2 - A + floor(A / 4.0);
  float jd = floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) + day + B - 1524.5;
  return jd;
}

float Dusk2Dawn::fractionOfCentury(float jd) {
  return (jd - 2451545.0) / 36525.0;
}

float Dusk2Dawn::equationOfTime(float t) {
  float epsilon = meanObliquityOfEcliptic(t);
  float l0 = geomMeanLongSun(t);
  float e = eccentricityEarthOrbit(t);
  float m = geomMeanAnomalySun(t);
  float y = tan(degToRad(epsilon) / 2.0);
  y *= y;
  float sin2l0 = sin(2.0 * degToRad(l0));
  float sinm = sin(degToRad(m));
  float cos2l0 = cos(2.0 * degToRad(l0));
  float sin4l0 = sin(4.0 * degToRad(l0));
  float sin2m = sin(2.0 * degToRad(m));
  float Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;
  return radToDeg(Etime) * 4.0;
}

float Dusk2Dawn::sunDeclination(float t) {
  return radToDeg(asin(sin(degToRad(obliquityCorrection(t))) * sin(degToRad(sunApparentLong(t)))));
}

float Dusk2Dawn::hourAngleSunrise(float lat, float solarDec) {
  float latRad = degToRad(lat);
  float sdRad = degToRad(solarDec);
  return acos(cos(degToRad(90.833)) / (cos(latRad) * cos(sdRad)) - tan(latRad) * tan(sdRad));
}

float Dusk2Dawn::obliquityCorrection(float t) {
  return meanObliquityOfEcliptic(t) + 0.00256 * cos(degToRad(125.04 - 1934.136 * t));
}

float Dusk2Dawn::meanObliquityOfEcliptic(float t) {
  float seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * (0.001813)));
  float e0 = 23.0 + (26.0 + (seconds / 60.0)) / 60.0;
  return e0;
}

float Dusk2Dawn::eccentricityEarthOrbit(float t) {
  return 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
}

float Dusk2Dawn::sunApparentLong(float t) {
  return sunTrueLong(t) - 0.00569 - 0.00478 * sin(degToRad(125.04 - 1934.136 * t));
}

float Dusk2Dawn::sunTrueLong(float t) {
  return geomMeanLongSun(t) + sunEqOfCenter(t);
}

float Dusk2Dawn::sunEqOfCenter(float t) {
  float m = geomMeanAnomalySun(t);
  float mrad = degToRad(m);
  float sinm = sin(mrad);
  float sin2m = sin(mrad + mrad);
  float sin3m = sin(mrad + mrad + mrad);
  return sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
}

float Dusk2Dawn::geomMeanLongSun(float t) {
  float L0 = 280.46646 + t * (36000.76983 + t * 0.0003032);
  while(L0 > 360.0) L0 -= 360.0;
  while(L0 < 0.0) L0 += 360.0;
  return L0;
}

float Dusk2Dawn::geomMeanAnomalySun(float t) {
  return 357.52911 + t * (35999.05029 - 0.0001537 * t);
}

float Dusk2Dawn::radToDeg(float rad) {
  return rad * 180.0 / PI;
}

float Dusk2Dawn::degToRad(float deg) {
  return deg * PI / 180.0;
}

bool Dusk2Dawn::min2str(char *str, int minutes) {
  int hours = minutes / 60;
  minutes = minutes % 60;
  if (hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59) {
    sprintf(str, "%02d:%02d", hours, minutes);
    return true;
  }
  return false;
}

bool Dusk2Dawn::zeroPadTime(char *str, byte digit) {
  if (digit < 0 || digit > 99) return false;
  sprintf(str, "%02d", digit);
  return true;
}