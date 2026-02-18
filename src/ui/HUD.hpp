/*
 * ui/HUD.hpp
 * Interfaz de usuario en partida: puntuación, vidas, modo de renderizado
 * y barra de boost con indicación de estado (READY/CHARGING/FIRING).
 */
#pragma once
#include <SDL2/SDL.h>
#include "../core/HyperspaceSystem.hpp"

class HUD {
public:
    HUD(int screenW, int screenH);

    /* Dibuja el HUD completo con los valores actuales del juego. */
    void draw(SDL_Renderer* rend, int score, int lives,
              bool hdMode, const HyperspaceSystem& boost) const;

private:
    int m_w, m_h;

    void drawLives(SDL_Renderer* rend, int lives, int x, int y) const;
    void drawBoostBar(SDL_Renderer* rend, const HyperspaceSystem& boost,
                      int x, int y) const;
};