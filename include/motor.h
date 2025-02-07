#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <RTClib.h>
#include <Timezone.h>

void stopPorte();
void ouvrirPorte();
void fermerPorte();
void handleButtonPress();
void initPorte();

#endif // MOTOR_H
