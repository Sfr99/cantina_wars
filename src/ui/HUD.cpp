#include "HUD.hpp"
#include "BitmapFont.hpp"
#include <string>
#include <algorithm>

HUD::HUD(int screenW, int screenH) : m_w(screenW), m_h(screenH) {}

void HUD::draw(SDL_Renderer* rend, int score, int lives,
               bool hdMode, const HyperspaceSystem& boost) const {
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    std::string scoreStr = "SCORE " + std::to_string(score);
    BitmapFont::drawText(rend, scoreStr.c_str(), 16, 14, 2, {0, 220, 255, 220});

    const char* modeStr = hdMode ? "HD" : "LO";
    int modeX = m_w - BitmapFont::textWidth(modeStr, 2) - 16;
    BitmapFont::drawText(rend, modeStr, modeX, 14, 2, {80, 120, 160, 180});

    drawLives(rend, lives, 16, 42);
    drawBoostBar(rend, boost, 16, m_h - 36);
}

void HUD::drawLives(SDL_Renderer* rend, int lives, int x, int y) const {
    BitmapFont::drawText(rend, "LIVES", x, y, 1, {120, 160, 180, 180});
    int cx = x + BitmapFont::textWidth("LIVES", 1) + 8;
    for (int i = 0; i < lives; i++) {
        int tx = cx + i * 14;
        SDL_SetRenderDrawColor(rend, 0, 210, 255, 200);
        SDL_RenderDrawLine(rend, tx + 5, y,      tx + 5,  y);
        SDL_RenderDrawLine(rend, tx + 5, y,      tx,      y + 9);
        SDL_RenderDrawLine(rend, tx + 5, y,      tx + 10, y + 9);
        SDL_RenderDrawLine(rend, tx,     y + 9,  tx + 10, y + 9);
        SDL_RenderDrawLine(rend, tx + 2, y + 9,  tx + 2,  y + 11);
        SDL_RenderDrawLine(rend, tx + 8, y + 9,  tx + 8,  y + 11);
    }
}

/* Dibuja la barra de boost con estado visual claro para cada fase. */
void HUD::drawBoostBar(SDL_Renderer* rend, const HyperspaceSystem& boost,
                       int x, int y) const {
    const int BAR_W = 140;
    const int BAR_H = 10;
    using State = HyperspaceSystem::State;

    // Etiqueta y color según fase
    const char* label;
    SDL_Color   fillColor;

    if (boost.firing) {
        label     = "BOOST";
        fillColor = { (Uint8)(0 + boost.charge * 180),
                      (Uint8)(200 + boost.charge * 55),
                      255, 220 };                       // cyan -> blanco al vaciarse
    } else if (boost.state == State::READY_TO_FIRE) {
        label     = "READY  HOLD SPACE";
        fillColor = { 0, 200, 120, 200 };               // verde: listo para usar
    } else {
        label     = "CHARGING";
        fillColor = { 60, 120, 220, 200 };              // azul: cargando
    }

    BitmapFont::drawText(rend, label, x, y - 12, 1, {100, 160, 200, 180});

    // Fondo y borde
    SDL_Rect bg{ x, y, BAR_W, BAR_H };
    SDL_SetRenderDrawColor(rend, 10, 20, 40, 180);
    SDL_RenderFillRect(rend, &bg);
    SDL_SetRenderDrawColor(rend, 0, 100, 160, 200);
    SDL_RenderDrawRect(rend, &bg);

    // Relleno proporcional a charge
    int fillW = (int)(boost.charge * (BAR_W - 2));
    if (fillW > 0) {
        SDL_Rect fill{ x + 1, y + 1, fillW, BAR_H - 2 };
        SDL_SetRenderDrawColor(rend, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
        SDL_RenderFillRect(rend, &fill);
    }
}