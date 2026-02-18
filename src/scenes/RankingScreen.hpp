/*
 * scenes/RankingScreen.hpp
 * Pantalla de tabla de puntuaciones top 5. Misma arquitectura de fondo
 * dinámico que DifficultyScreen (callback desde MainMenu).
 */
#pragma once
#include <SDL2/SDL.h>
#include <functional>
#include "../core/RankingSystem.hpp"
#include "../rendering/Renderer.hpp"

class RankingScreen {
public:
    RankingScreen(Renderer& renderer, const RankingSystem& ranking,
                  std::function<void()> bgCallback = nullptr,
                  int highlightScore = -1);

    /* Bloquea hasta que el usuario pulsa ESC, Enter o hace click. */
    void run();

private:
    Renderer&             m_renderer;
    const RankingSystem&  m_ranking;
    std::function<void()> m_bgCallback;
    int                   m_highlightScore; // resalta esta puntuación si está en tabla
    bool                  m_done = false;

    void handleEvents();
    void render();
};