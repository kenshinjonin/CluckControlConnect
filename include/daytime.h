/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * daytime.h
 * ---------
 * Décision jour/nuit de l'asservissement de la porte, sans dépendance Arduino
 * pour être testée sur PC (test/test_daytime).
 */

#pragma once

// Vrai entre l'ouverture (incluse) et la fermeture (exclue), en minutes depuis
// minuit. Les décalages configurables peuvent faire passer la fermeture après
// minuit : la comparaison est alors traitée en intervalle circulaire.
inline bool isDaytime(int sunrise, int sunset, int now) {
    if (sunset > sunrise) return now >= sunrise && now < sunset;
    return now >= sunrise || now < sunset;
}
