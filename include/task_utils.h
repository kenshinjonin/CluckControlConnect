/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * task_utils.h
 * ------------
 * Utilitaires partagés par les tâches FreeRTOS.
 */

#pragma once

#include <Arduino.h>
#include <esp_task_wdt.h>

// Sommeil fractionné : la tâche se réveille toutes les secondes uniquement pour
// réarmer le watchdog. Cela permet de garder un timeout de watchdog court
// (donc une détection de blocage rapide) malgré des périodes d'attente longues.
inline void watchdogDelay(uint32_t totalMs) {
    const uint32_t chunkMs = 1000;
    while (totalMs > 0) {
        uint32_t slice = totalMs > chunkMs ? chunkMs : totalMs;
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(slice));
        totalMs -= slice;
    }
    esp_task_wdt_reset();
}
