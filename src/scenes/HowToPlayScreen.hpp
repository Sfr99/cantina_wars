/*
 * scenes/HowToPlayScreen.hpp
 * Pantalla de controles e instrucciones. Pantalla estática con fondo dinámico.
 */
#pragma once
#include <SDL2/SDL.h>
#include <functional>
#include "../rendering/Renderer.hpp"

class HowToPlayScreen {
public:
    HowToPlayScreen(Renderer& renderer, std::function<void()> bgCallback = nullptr);
    void run();

private:
    Renderer&             m_renderer;
    std::function<void()> m_bgCallback;
    bool                  m_done = false;

    void handleEvents();
    void render();
};