/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * daytime.h
 * ---------
 * Décision jour/nuit de l'asservissement de la porte et conversion en heure
 * locale, sans dépendance Arduino pour être testées sur PC (test/test_daytime).
 */

#pragma once

#include <time.h>

// Avant 2024-01-01 UTC, l'horloge n'a pas été réglée (ni NTP ni RTC)
static const time_t MIN_VALID_EPOCH = 1704067200;

// Minutes depuis minuit, en heure locale (fuseau et heure d'été de la variable TZ)
inline int localMinutesOf(time_t t) {
    struct tm local;
    localtime_r(&t, &local);
    return local.tm_hour * 60 + local.tm_min;
}

// Vrai entre l'ouverture (incluse) et la fermeture (exclue), en minutes depuis
// minuit. Les décalages configurables peuvent faire passer la fermeture après
// minuit : la comparaison est alors traitée en intervalle circulaire.
inline bool isDaytime(int sunrise, int sunset, int now) {
    if (sunset > sunrise) return now >= sunrise && now < sunset;
    return now >= sunrise || now < sunset;
}
