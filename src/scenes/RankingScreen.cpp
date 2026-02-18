#include "RankingScreen.hpp"
#include "../ui/BitmapFont.hpp"
#include <string>

static constexpr int W  = 900;
static constexpr int H  = 600;
static constexpr int CX = W / 2;

RankingScreen::RankingScreen(Renderer& renderer, const RankingSystem& ranking,
                             std::function<void()> bgCallback, int highlightScore)
    : m_renderer(renderer), m_ranking(ranking),
      m_bgCallback(bgCallback), m_highlightScore(highlightScore) {}

void RankingScreen::run() {
    m_done = false;
    while (!m_done) {
        handleEvents();
        render();
    }
}

void RankingScreen::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { m_done = true; return; }
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                case SDLK_SPACE:
                    m_done = true;
                    break;
            }
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) m_done = true;
    }
}

void RankingScreen::render() {
    SDL_Renderer* rend = m_renderer.sdlRenderer();

    if (m_bgCallback) {
        m_bgCallback();
    } else {
        SDL_SetRenderDrawColor(rend, 2, 5, 15, 255);
        SDL_RenderClear(rend);
    }

    // Overlay semitransparente
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 180);
    SDL_Rect full{ 0, 0, W, H };
    SDL_RenderFillRect(rend, &full);

    // Título
    BitmapFont::drawTextCentered(rend, "RANKING", CX, 70, 4, {0, 220, 255, 255});
    BitmapFont::drawTextCentered(rend, "TOP 5", CX, 125, 1, {80, 130, 160, 200});

    // Panel de la tabla
    const int PANEL_W = 440;
    const int PANEL_H = 280;
    const int PANEL_X = CX - PANEL_W / 2;
    const int PANEL_Y = 155;
    SDL_Rect panel{ PANEL_X, PANEL_Y, PANEL_W, PANEL_H };
    SDL_SetRenderDrawColor(rend, 5, 10, 30, 210);
    SDL_RenderFillRect(rend, &panel);
    SDL_SetRenderDrawColor(rend, 0, 120, 180, 180);
    SDL_RenderDrawRect(rend, &panel);

    // Cabecera
    const int COL_RANK  = PANEL_X + 24;
    const int COL_NAME  = PANEL_X + 80;
    const int COL_SCORE = PANEL_X + PANEL_W - 24;
    const int ROW_START = PANEL_Y + 20;
    const int ROW_H     = 46;

    BitmapFont::drawText(rend, "RK", COL_RANK,  ROW_START, 1, {80, 120, 150, 180});
    BitmapFont::drawText(rend, "NOMBRE",  COL_NAME,  ROW_START, 1, {80, 120, 150, 180});
    int scoreHdrX = COL_SCORE - BitmapFont::textWidth("PUNTOS", 1);
    BitmapFont::drawText(rend, "PUNTOS", scoreHdrX, ROW_START, 1, {80, 120, 150, 180});

    // Separador
    SDL_SetRenderDrawColor(rend, 0, 100, 150, 140);
    SDL_RenderDrawLine(rend, PANEL_X + 10, ROW_START + 12,
                             PANEL_X + PANEL_W - 10, ROW_START + 12);

    // Filas
    const auto& entries = m_ranking.entries();
    const char* medals[] = { "1", "2", "3", "4", "5" };
    SDL_Color medalColors[] = {
        {255, 215,   0, 255},  // oro
        {200, 200, 200, 255},  // plata
        {200, 130,  50, 255},  // bronce
        {160, 180, 200, 220},
        {140, 160, 180, 200},
    };

    for (int i = 0; i < RankingSystem::MAX_ENTRIES; i++) {
        int rowY = ROW_START + 20 + i * ROW_H;
        bool isNew = (i < (int)entries.size()) &&
                     (entries[i].score == m_highlightScore) &&
                     m_highlightScore > 0;

        // Fondo resaltado para el nuevo score
        if (isNew) {
            SDL_Rect hl{ PANEL_X + 4, rowY - 4, PANEL_W - 8, ROW_H - 4 };
            SDL_SetRenderDrawColor(rend, 0, 60, 100, 140);
            SDL_RenderFillRect(rend, &hl);
            SDL_SetRenderDrawColor(rend, 0, 200, 255, 180);
            SDL_RenderDrawRect(rend, &hl);
        }

        SDL_Color col = (i < (int)medalColors->r) ? medalColors[std::min(i, 4)]
                                                   : SDL_Color{140, 160, 180, 200};
        col = medalColors[std::min(i, 4)];

        // Rango
        BitmapFont::drawText(rend, medals[i], COL_RANK, rowY + 8, 2, col);

        if (i < (int)entries.size()) {
            // Nombre
            char nameBuf[4] = { entries[i].name[0], entries[i].name[1],
                                 entries[i].name[2], '\0' };
            BitmapFont::drawText(rend, nameBuf, COL_NAME, rowY + 8, 2,
                                 isNew ? SDL_Color{0, 240, 255, 255} : col);

            // Puntuación (alineada a la derecha)
            std::string sc = std::to_string(entries[i].score);
            int scX = COL_SCORE - BitmapFont::textWidth(sc.c_str(), 2);
            BitmapFont::drawText(rend, sc.c_str(), scX, rowY + 8, 2,
                                 isNew ? SDL_Color{0, 240, 255, 255} : col);

            // Etiqueta NEW en el resaltado
            if (isNew) {
                BitmapFont::drawText(rend, "NEW", PANEL_X + PANEL_W - 52,
                                     rowY + 8, 1, {0, 240, 255, 255});
            }
        } else {
            // Slot vacío
            BitmapFont::drawText(rend, "---", COL_NAME,   rowY + 8, 2, {50, 70, 90, 150});
            BitmapFont::drawText(rend, "---", COL_SCORE - BitmapFont::textWidth("---", 2),
                                 rowY + 8, 2, {50, 70, 90, 150});
        }
    }

    // Instrucción de salida
    BitmapFont::drawTextCentered(rend, "PULSA CUALQUIER TECLA PARA VOLVER",
                                 CX, H - 30, 1, {60, 80, 100, 180});

    SDL_RenderPresent(rend);
}