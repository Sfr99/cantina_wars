/*
 * ui/Overlay.hpp
 * Overlay semitransparente reutilizable. Soporta dos modos:
 *   - INFO: título + subtítulo + acción parpadeante (Game Over simple)
 *   - NAME_ENTRY: entrada de nombre de 3 letras estilo recreativa
 */
#pragma once
#include <SDL2/SDL.h>
#include <string>

class Overlay {
public:
    Overlay(int screenW, int screenH);

    /* Dibuja overlay informativo: fondo + título + subtítulo + acción parpadeante. */
    void draw(SDL_Renderer* rend,
              const char* title,
              const char* subtitle,
              const char* action) const;

    /* Dibuja overlay de entrada de nombre. name[3] = letras actuales, cursor = slot activo [0,2]. */
    void drawNameEntry(SDL_Renderer* rend,
                       const char* title,
                       const char* scoreStr,
                       const char name[3],
                       int cursor) const;

private:
    int m_w, m_h;

    /* Dibuja el panel central con fondo y borde. Devuelve (cx, cy). */
    void drawPanel(SDL_Renderer* rend, int panelW, int panelH,
                   int& outCx, int& outCy) const;
};