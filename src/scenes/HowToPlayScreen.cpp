#include "HowToPlayScreen.hpp"
#include "../ui/BitmapFont.hpp"

static constexpr int W  = 900;
static constexpr int H  = 600;
static constexpr int CX = W / 2;

HowToPlayScreen::HowToPlayScreen(Renderer& renderer, std::function<void()> bgCallback)
    : m_renderer(renderer), m_bgCallback(bgCallback) {}

void HowToPlayScreen::run() {
    m_done = false;
    while (!m_done) {
        handleEvents();
        render();
    }
}

void HowToPlayScreen::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)       { m_done = true; return; }
        if (e.type == SDL_KEYDOWN)    { m_done = true; return; }
        if (e.type == SDL_MOUSEBUTTONDOWN) { m_done = true; return; }
    }
}

/* Dibuja una fila de sección: icono de tecla + descripción. */
static void drawRow(SDL_Renderer* rend, int x, int y,
                    const char* key, const char* desc,
                    SDL_Color keyCol, SDL_Color descCol) {
    // Caja de tecla
    int keyW = BitmapFont::textWidth(key, 1) + 10;
    SDL_Rect box{ x, y - 1, keyW, 11 };
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, 20, 40, 80, 180);
    SDL_RenderFillRect(rend, &box);
    SDL_SetRenderDrawColor(rend, keyCol.r, keyCol.g, keyCol.b, 200);
    SDL_RenderDrawRect(rend, &box);
    BitmapFont::drawText(rend, key, x + 5, y, 1, keyCol);

    // Descripción
    BitmapFont::drawText(rend, desc, x + keyW + 10, y, 1, descCol);
}

void HowToPlayScreen::render() {
    SDL_Renderer* rend = m_renderer.sdlRenderer();

    if (m_bgCallback) {
        m_bgCallback();
    } else {
        SDL_SetRenderDrawColor(rend, 2, 5, 15, 255);
        SDL_RenderClear(rend);
    }

    // Overlay
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 190);
    SDL_Rect full{ 0, 0, W, H };
    SDL_RenderFillRect(rend, &full);

    // Título
    BitmapFont::drawTextCentered(rend, "COMO JUGAR", CX, 40, 4, {0, 220, 255, 255});

    // ── Dos columnas ──────────────────────────────────────────────
    const int COL1 = 80;
    const int COL2 = 490;
    const int ROW0 = 130;
    const int STEP = 18;

    SDL_Color secCol  = {0, 180, 220, 255};
    SDL_Color keyCol  = {0, 220, 255, 255};
    SDL_Color descCol = {180, 210, 230, 220};
    SDL_Color dimCol  = {120, 150, 170, 180};

    // ── Columna izquierda: controles ──
    BitmapFont::drawText(rend, "MOVIMIENTO", COL1, ROW0, 1, secCol);
    SDL_SetRenderDrawColor(rend, 0, 140, 180, 160);
    SDL_RenderDrawLine(rend, COL1, ROW0 + 10, COL1 + 160, ROW0 + 10);

    int y = ROW0 + 18;
    drawRow(rend, COL1,      y, "W  /  UP",    "INCLINAR ARRIBA",   keyCol, descCol); y += STEP;
    drawRow(rend, COL1,      y, "S  /  DOWN",  "INCLINAR ABAJO",    keyCol, descCol); y += STEP;
    drawRow(rend, COL1,      y, "A  /  LEFT",  "INCLINAR IZQUIERDA",keyCol, descCol); y += STEP;
    drawRow(rend, COL1,      y, "D  /  RIGHT", "INCLINAR DERECHA",  keyCol, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "COMBATE", COL1, y, 1, secCol);
    SDL_SetRenderDrawColor(rend, 0, 140, 180, 160);
    SDL_RenderDrawLine(rend, COL1, y + 10, COL1 + 160, y + 10);
    y += 18;

    drawRow(rend, COL1, y, "F", "DISPARAR", keyCol, descCol); y += STEP;
    drawRow(rend, COL1, y, "SPACE", "BOOST  (mantener)", keyCol, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "SISTEMA", COL1, y, 1, secCol);
    SDL_SetRenderDrawColor(rend, 0, 140, 180, 160);
    SDL_RenderDrawLine(rend, COL1, y + 10, COL1 + 160, y + 10);
    y += 18;

    drawRow(rend, COL1, y, "T",   "ALTERNAR MODO HD / LO", keyCol, descCol); y += STEP;
    drawRow(rend, COL1, y, "R",   "REINICIAR (game over)",  keyCol, descCol); y += STEP;
    drawRow(rend, COL1, y, "ESC", "SALIR AL MENU",          keyCol, descCol);

    // ── Columna derecha: mecánicas ──
    BitmapFont::drawText(rend, "MECANICAS", COL2, ROW0, 1, secCol);
    SDL_SetRenderDrawColor(rend, 0, 140, 180, 160);
    SDL_RenderDrawLine(rend, COL2, ROW0 + 10, COL2 + 320, ROW0 + 10);

    y = ROW0 + 18;
    BitmapFont::drawText(rend, "OBJETIVO", COL2, y, 1, keyCol); y += STEP;
    BitmapFont::drawText(rend, "SOBREVIVE Y CONSIGUE", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "LA MAXIMA PUNTUACION.", COL2, y, 1, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "PUNTUACION", COL2, y, 1, keyCol); y += STEP;
    BitmapFont::drawText(rend, "DISTANCIA RECORRIDA  +  ASTEROIDES", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "DESTRUIDOS (20 / 50 / 100 PTS)", COL2, y, 1, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "BOOST", COL2, y, 1, keyCol); y += STEP;
    BitmapFont::drawText(rend, "LA BARRA SE CARGA SOLA. AL LLEGAR", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "AL MAX MANTENER SPACE PARA ACTIVAR.", COL2, y, 1, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "VIDAS", COL2, y, 1, keyCol); y += STEP;
    BitmapFont::drawText(rend, "EMPIEZAS CON 3. AL GOLPEAR UN", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "ASTEROIDE PIERDES UNA. TRAS MORIR", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "ERES INVENCIBLE UNOS SEGUNDOS.", COL2, y, 1, descCol); y += STEP + 6;

    BitmapFont::drawText(rend, "RECORD", COL2, y, 1, keyCol); y += STEP;
    BitmapFont::drawText(rend, "SI ENTRAS EN EL TOP 5 PODRAS", COL2, y, 1, descCol); y += STEP - 4;
    BitmapFont::drawText(rend, "INTRODUCIR TUS INICIALES.", COL2, y, 1, descCol);

    // Pie
    bool blink = (SDL_GetTicks() / 500) % 2 == 0;
    if (blink)
        BitmapFont::drawTextCentered(rend, "PULSA CUALQUIER TECLA PARA VOLVER",
                                     CX, H - 28, 1, {80, 110, 140, 200});

    SDL_RenderPresent(rend);
}