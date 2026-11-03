/*
 * CluckControl Connect © 2025 - Auteur : Kenshinjonin
 * Licence : MIT
 *
 * login_guard.h
 * -------------
 * Limitation des essais de mot de passe, par adresse IP.
 *
 * Une adresse qui accumule MAX_FAILURES échecs est refusée pendant LOCK_MS,
 * même avec le bon mot de passe : sinon un script continuerait de deviner à
 * pleine vitesse. Le verrouillage étant par adresse, un attaquant ne bloque
 * que lui-même. Seul cas où une adresse inconnue est refusée : la table est
 * pleine d'adresses en échec, c'est-à-dire une attaque depuis plusieurs postes.
 *
 * Sans dépendance Arduino (l'heure est passée en paramètre) : testable sur PC.
 */

#pragma once

#include <stdint.h>

class LoginGuard {
public:
    static const uint8_t MAX_FAILURES = 5;
    static const uint32_t LOCK_MS = 5UL * 60UL * 1000UL;

    static bool isLocked(uint32_t ip, uint32_t now) {
        Slot *slot = find(ip, now);
        return slot == nullptr || slot->failures >= MAX_FAILURES;
    }

    static void recordFailure(uint32_t ip, uint32_t now) {
        Slot *slot = find(ip, now);
        if (slot == nullptr) return;
        slot->ip = ip;
        if (slot->failures < MAX_FAILURES) slot->failures++;
        slot->last = now;
    }

    static void clear(uint32_t ip, uint32_t now) {
        Slot *slot = find(ip, now);
        if (slot != nullptr) slot->failures = 0;
    }

private:
    static const int SLOT_COUNT = 4;

    struct Slot {
        uint32_t ip;
        uint8_t failures;
        uint32_t last;
    };

    // Retourne l'entrée de cette adresse, sinon une entrée libre, sinon nullptr
    // (table pleine). Les entrées expirées sont libérées au passage.
    static Slot *find(uint32_t ip, uint32_t now) {
        static Slot slots[SLOT_COUNT] = {};
        Slot *freeSlot = nullptr;
        for (Slot &slot : slots) {
            if (slot.failures > 0 && now - slot.last > LOCK_MS) slot.failures = 0;
            if (slot.failures > 0 && slot.ip == ip) return &slot;
            if (slot.failures == 0 && freeSlot == nullptr) freeSlot = &slot;
        }
        return freeSlot;
    }
};
