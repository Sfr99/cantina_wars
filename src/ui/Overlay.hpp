/*
 * ui/Overlay.hpp
 * Overlay semitransparente reutilizable para Game Over, pausa, etc.
 * Dibuja un fondo oscurecido con título, subtítulo y una línea de acción.
 */
#pragma once
#include <SDL2/SDL.h>
#include <string>

class Overlay {
public:
    Overlay(int screenW, int screenH);

    /* Dibuja el overlay completo: fondo + título + subtítulo + acción. */
    void draw(SDL_Renderer* rend,
              const char* title,
              const char* subtitle,
              const char* action) const;

private:
    int m_w, m_h;
};