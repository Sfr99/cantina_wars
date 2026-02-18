/*
 * core/HyperspaceSystem.hpp
 * Máquina de estados para el turbo/hyperspace de la nave. Gestiona la carga
 * y decaimiento del impulso y expone la intensidad normalizada [0,1].
 */
#pragma once

struct HyperspaceSystem {
    float charge     = 0.f;
    float chargeTime = 1.0f;   // segundos hasta carga completa
    float decayTime  = 0.5f;   // segundos hasta descarga completa
    float maxMult    = 3.0f;   // multiplicador máximo de velocidad

    /* Avanza la carga hacia target (1=boost activo, 0=sin boost) según dt. */
    void update(bool boostHeld, float dt) {
        float target = boostHeld ? 1.f : 0.f;
        float time   = boostHeld ? chargeTime : decayTime;
        if (time <= 1e-6f) { charge = target; return; }
        float step = dt / time;
        if (charge < target) charge = std::min(charge + step, target);
        else                 charge = std::max(charge - step, target);
    }

    /* Multiplicador de velocidad forward interpolado entre 1 y maxMult. */
    float speedMultiplier() const { return 1.f + charge * (maxMult - 1.f); }
};