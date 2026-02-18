#include "Overlay.hpp"
#include "BitmapFont.hpp"

Overlay::Overlay(int screenW, int screenH) : m_w(screenW), m_h(screenH) {}

/* Dibuja fondo oscurecido + panel central. Devuelve el centro del panel. */
void Overlay::drawPanel(SDL_Renderer* rend, int panelW, int panelH,
                        int& outCx, int& outCy) const {
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    SDL_Rect screen{ 0, 0, m_w, m_h };
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 180);
    SDL_RenderFillRect(rend, &screen);

    outCx = m_w / 2;
    outCy = m_h / 2;

    SDL_Rect panel{ outCx - panelW/2, outCy - panelH/2, panelW, panelH };
    SDL_SetRenderDrawColor(rend, 5, 10, 30, 220);
    SDL_RenderFillRect(rend, &panel);
    SDL_SetRenderDrawColor(rend, 0, 180, 255, 200);
    SDL_RenderDrawRect(rend, &panel);
}

/* Overlay informativo estándar. */
void Overlay::draw(SDL_Renderer* rend,
                   const char* title,
                   const char* subtitle,
                   const char* action) const {
    int cx, cy;
    drawPanel(rend, 420, 180, cx, cy);

    BitmapFont::drawTextCentered(rend, title,    cx, cy - 60, 4, {0, 220, 255, 255});
    BitmapFont::drawTextCentered(rend, subtitle, cx, cy - 10, 2, {180, 220, 240, 220});

    bool blink = (SDL_GetTicks() / 500) % 2 == 0;
    if (blink)
        BitmapFont::drawTextCentered(rend, action, cx, cy + 40, 1, {120, 180, 200, 200});
}

/* Overlay de entrada de nombre estilo recreativa.
   Muestra tres slots de letra con cursor animado y flechas de ayuda. */
void Overlay::drawNameEntry(SDL_Renderer* rend,
                            const char* title,
                            const char* scoreStr,
                            const char name[3],
                            int cursor) const {
    int cx, cy;
    drawPanel(rend, 460, 220, cx, cy);

    // Título
    BitmapFont::drawTextCentered(rend, title, cx, cy - 85, 3, {0, 220, 255, 255});

    // Score
    BitmapFont::drawTextCentered(rend, scoreStr, cx, cy - 45, 2, {180, 220, 240, 220});

    // Etiqueta
    BitmapFont::drawTextCentered(rend, "INTRODUCE TU NOMBRE", cx, cy - 10, 1, {120, 160, 190, 180});

    // Tres slots de letra
    const int SLOT_W   = 40;
    const int SLOT_H   = 50;
    const int SLOT_GAP = 16;
    const int TOTAL    = SLOT_W * 3 + SLOT_GAP * 2;
    int slotX = cx - TOTAL / 2;
    const int slotY = cy + 10;

    for (int i = 0; i < 3; i++) {
        SDL_Rect slot{ slotX, slotY, SLOT_W, SLOT_H };
        bool active = (i == cursor);

        // Fondo del slot
        SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(rend, active ? 10 : 5, active ? 50 : 15,
                               active ? 100 : 30, active ? 220 : 160);
        SDL_RenderFillRect(rend, &slot);

        // Borde — parpadeante en el slot activo
        bool blink = (SDL_GetTicks() / 300) % 2 == 0;
        SDL_Color borderCol = active && blink ? SDL_Color{0, 255, 255, 255}
                                              : SDL_Color{0, 140, 200, 200};
        SDL_SetRenderDrawColor(rend, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
        SDL_RenderDrawRect(rend, &slot);

        // Letra centrada en el slot
        char buf[2] = { name[i], '\0' };
        int letterX = slotX + SLOT_W/2 - BitmapFont::textWidth(buf, 3)/2;
        int letterY = slotY + (SLOT_H - BitmapFont::GLYPH_H * 3) / 2;
        BitmapFont::drawText(rend, buf, letterX, letterY, 3,
                             active ? SDL_Color{0, 240, 255, 255}
                                    : SDL_Color{180, 210, 230, 220});

        // Flecha arriba/abajo en slot activo
        if (active) {
            BitmapFont::drawTextCentered(rend, "^", slotX + SLOT_W/2, slotY - 14, 1,
                                         {0, 200, 220, 180});
            BitmapFont::drawTextCentered(rend, "V", slotX + SLOT_W/2, slotY + SLOT_H + 4, 1,
                                         {0, 200, 220, 180});
        }

        slotX += SLOT_W + SLOT_GAP;
    }

    // Instrucciones
    BitmapFont::drawTextCentered(rend, "FLECHAS CAMBIAR LETRA   ENTER CONFIRMAR",
                                 cx, cy + 80, 1, {80, 110, 140, 180});
}