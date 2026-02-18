#include "Overlay.hpp"
#include "BitmapFont.hpp"

Overlay::Overlay(int screenW, int screenH) : m_w(screenW), m_h(screenH) {}

/* Dibuja fondo semitransparente + texto centrado en tres niveles de jerarquía. */
void Overlay::draw(SDL_Renderer* rend,
                   const char* title,
                   const char* subtitle,
                   const char* action) const {
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    // Fondo oscurecido
    SDL_Rect screen{ 0, 0, m_w, m_h };
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 180);
    SDL_RenderFillRect(rend, &screen);

    const int cx = m_w / 2;
    const int cy = m_h / 2;

    // Panel central
    const int PANEL_W = 420, PANEL_H = 180;
    SDL_Rect panel{ cx - PANEL_W/2, cy - PANEL_H/2, PANEL_W, PANEL_H };
    SDL_SetRenderDrawColor(rend, 5, 10, 30, 220);
    SDL_RenderFillRect(rend, &panel);
    SDL_SetRenderDrawColor(rend, 0, 180, 255, 200);
    SDL_RenderDrawRect(rend, &panel);

    // Título grande
    BitmapFont::drawTextCentered(rend, title, cx, cy - 60, 4, {0, 220, 255, 255});

    // Subtítulo (score, etc.)
    BitmapFont::drawTextCentered(rend, subtitle, cx, cy - 10, 2, {180, 220, 240, 220});

    // Línea de acción parpadeante usando el tick de SDL
    bool blink = (SDL_GetTicks() / 500) % 2 == 0;
    if (blink)
        BitmapFont::drawTextCentered(rend, action, cx, cy + 40, 1, {120, 180, 200, 200});
}