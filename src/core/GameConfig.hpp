/*
 * core/GameConfig.hpp
 * Configuración compartida entre escenas. Se crea en main, se escribe desde
 * los menús (dificultad, configuración) y se lee desde Game al iniciar.
 * Sin dependencias externas; es un POD con valores por defecto.
 */
#pragma once
#include <array>

enum class Difficulty { EASY = 0, NORMAL = 1, HARD = 2 };

struct GameConfig {
    Difficulty difficulty = Difficulty::NORMAL;

    float musicVol = 0.8f;  // [0, 1]
    float sfxVol   = 0.8f;  // [0, 1]

    // Top 5 scores (ordenados desc). Se persisten en scores.txt via RankingSystem.
    static constexpr int MAX_SCORES = 5;
    std::array<int, MAX_SCORES> topScores = {};

    // Parámetros derivados de dificultad — llamar tras cambiar difficulty.
    struct DifficultyParams {
        float asteroidSpeedMult;   // multiplicador de velocidad de asteroides
        float spawnRateMult;       // multiplicador de frecuencia de spawn
        float boostChargeTime;     // segundos para cargar el boost
        int   baseAsteroidCount;   // asteroides iniciales en escena
    };

    /* Devuelve los parámetros de juego para la dificultad actual. */
    DifficultyParams params() const {
        switch (difficulty) {
            case Difficulty::EASY:
                return { 0.65f, 0.60f, 14.0f, 10 };
            case Difficulty::HARD:
                return { 1.45f, 1.50f, 28.0f, 20 };
            default: // NORMAL
                return { 1.00f, 1.00f, 20.0f, 15 };
        }
    }
};