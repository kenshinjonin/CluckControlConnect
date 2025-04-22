#ifndef DUSK2DAWN_H
#define DUSK2DAWN_H

#include <Arduino.h>

class Dusk2Dawn {
public:
    Dusk2Dawn(float latitude, float longitude, float timezone);
    int sunrise(int year, int month, int day, bool isDST);
    int sunset(int year, int month, int day, bool isDST);
    static bool min2str(char *str, int minutes);

private:
    float _latitude;
    float _longitude;
    int _timezone;
    
    int sunriseSet(bool sunrise, int year, int month, int day, bool isDST);
    float sunriseSetUTC(bool sunrise, float jday, float latitude, float longitude);
    float equationOfTime(float t);
    float meanObliquityOfEcliptic(float t);
    float eccentricityEarthOrbit(float t);
    float sunDeclination(float t);
    float sunApparentLong(float t);
    float sunTrueLong(float t);
    float sunEqOfCenter(float t);
    float hourAngleSunrise(float lat, float solarDec);
    float obliquityCorrection(float t);
    float geomMeanLongSun(float t);
    float geomMeanAnomalySun(float t);
    float jDay(int year, int month, int day);
    float fractionOfCentury(float jd);
    float radToDeg(float rad);
    float degToRad(float deg);
    static bool zeroPadTime(char *str, byte digit);
};

#endif // DUSK2DAWN_H