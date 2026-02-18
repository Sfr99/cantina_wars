/*
 * core/HyperspaceSystem.hpp
 * State machine del boost/hyperspace.
 *
 * Ciclo:
 *   CHARGING  -> carga automática siempre. Al llegar a MAX entra en READY_TO_FIRE.
 *   READY_TO_FIRE -> carga completa, esperando input. Mantener SPACE activa el boost.
 *                   Al vaciarse del todo vuelve a CHARGING.
 *
 * Mientras SPACE esté pulsado en READY_TO_FIRE:
 *   - firing = true  -> velocidad x3, speed lines, audio
 * Al soltar SPACE:
 *   - firing = false -> pausa, la carga se recarga
 * Al llegar a 0:
 *   - vuelve a CHARGING automáticamente
 *
 * Audio: enteredFiring/leftFiring son flags de un solo frame para Game.
 */
#pragma once
#include <algorithm>

struct HyperspaceSystem {
    enum class State { CHARGING, READY_TO_FIRE };

    State state  = State::CHARGING;
    float charge = 0.f;           // [0, 1]
    bool  firing = false;         // true solo cuando SPACE está pulsado y hay carga

    float chargeTime = 20.0f;     // segundos para llegar a MAX desde 0
    float fireTime   = 11.0f;     // segundos para vaciarse del todo
    float maxMult    = 3.0f;      // multiplicador de velocidad al hacer boost

    // Flags de un solo frame para disparar/parar audio en Game
    bool enteredFiring = false;
    bool leftFiring    = false;

    /* Avanza la máquina de estados. boostHeld = SPACE pulsado este frame. */
    void update(bool boostHeld, float dt) {
        enteredFiring = false;
        leftFiring    = false;

        auto startFiringIfNeeded = [&]() {
            if (!firing) {
                firing = true;
                enteredFiring = true;
            }
        };

        auto stopFiringIfNeeded = [&]() {
            if (firing) {
                firing = false;
                leftFiring = true;
            }
        };

    switch (state) {
        case State::CHARGING: {
            // Si SPACE está pulsado y hay algo de carga, boost aunque no esté al 100%
            if (boostHeld && charge > 0.f) {
                startFiringIfNeeded();

                // Drenar carga
                charge = std::max(charge - dt / fireTime, 0.f);

                // Si se vacía, se corta el boost y seguimos cargando
                if (charge <= 0.f) {
                    charge = 0.f;
                    stopFiringIfNeeded();
                }
            } else {
                // Si no estamos boosteando, recargar
                stopFiringIfNeeded();
                charge = std::min(charge + dt / chargeTime, 1.f);
            }

            // Si llega a carga completa, entrar en READY (pero no hace falta forzar firing)
            if (charge >= 1.f) {
                charge = 1.f;
                state  = State::READY_TO_FIRE;
            }
            break;
        }

        case State::READY_TO_FIRE: {
            if (boostHeld && charge > 0.f) {
                startFiringIfNeeded();

                charge = std::max(charge - dt / fireTime, 0.f);
                if (charge <= 0.f) {
                    charge = 0.f;
                    stopFiringIfNeeded();
                    state  = State::CHARGING;
                }
            } else {
                // No hay boost: no disparar. 
                stopFiringIfNeeded();

                if (charge < 1.f) state = State::CHARGING;
            }
            break;
        }
    }
}

    /* Multiplicador de velocidad: >1 solo mientras firing. */
    float speedMultiplier() const {
        return firing ? (1.f + charge * (maxMult - 1.f)) : 1.f;
    }

    /* Intensidad visual [0,1]: pulso suave al cargar, plena al hacer boost. */
    float visualIntensity() const {
        if (firing)                        return charge;
        if (state == State::CHARGING)      return charge * 0.15f;
        if (state == State::READY_TO_FIRE) return 0.05f; // parpadeo suave "listo"
        return 0.f;
    }
};