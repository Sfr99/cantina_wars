#include "DifficultyScreen.hpp"
#include "../ui/BitmapFont.hpp"
#include <algorithm>

static constexpr int W  = 900;
static constexpr int H  = 600;
static constexpr int CX = W / 2;

// Tres opciones en horizontal, centradas
static constexpr int OPT_W   = 220;
static constexpr int OPT_H   = 120;
static constexpr int OPT_GAP = 24;
static constexpr int OPT_Y   = 220;
static constexpr int TOTAL_W = OPT_W * 3 + OPT_GAP * 2;
static constexpr int OPT_X0  = CX - TOTAL_W / 2;

DifficultyScreen::DifficultyScreen(Renderer& renderer, GameConfig& config,
                                     std::function<void()> bgCallback)
    : m_renderer(renderer), m_config(config), m_bgCallback(bgCallback) {
    m_selected = (int)config.difficulty;
    buildButtons();
}

void DifficultyScreen::buildButtons() {
    const char* labels[] = { "FACIL", "NORMAL", "DIFICIL" };
    for (int i = 0; i < 3; i++) {
        Button btn;
        btn.label   = labels[i];
        btn.enabled = true;
        btn.state   = ButtonState::IDLE;
        btn.rect    = { OPT_X0 + i * (OPT_W + OPT_GAP), OPT_Y, OPT_W, OPT_H };
        m_buttons.push_back(btn);
    }
}

const char* DifficultyScreen::description(Difficulty d) {
    switch (d) {
        case Difficulty::EASY:   return "ASTEROIDES LENTOS    BOOST RAPIDO";
        case Difficulty::HARD:   return "ASTEROIDES RAPIDOS   BOOST LENTO";
        default:                 return "VELOCIDAD ESTANDAR   BOOST NORMAL";
    }
}

void DifficultyScreen::run() {
    m_done = false;
    while (!m_done) {
        handleEvents();
        render();
    }
    // Escribir selección en config al salir
    m_config.difficulty = (Difficulty)m_selected;
}

void DifficultyScreen::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)  { m_done = true; return; }

        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                case SDLK_BACKSPACE:
                    m_done = true;
                    break;
                case SDLK_LEFT:
                    m_selected = std::max(0, m_selected - 1);
                    break;
                case SDLK_RIGHT:
                    m_selected = std::min(2, m_selected + 1);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    m_done = true;
                    break;
            }
        }

        if (e.type == SDL_MOUSEMOTION) {
            for (int i = 0; i < 3; i++) {
                if (m_buttons[i].contains(e.motion.x, e.motion.y))
                    m_selected = i;
            }
        }

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            for (int i = 0; i < 3; i++) {
                if (m_buttons[i].contains(e.button.x, e.button.y)) {
                    m_selected = i;
                    m_done     = true;
                }
            }
        }
    }
}

void DifficultyScreen::render() {
    SDL_Renderer* rend = m_renderer.sdlRenderer();

    // Dibujar el fondo (menú principal) si se proporcionó callback
    if (m_bgCallback) {
        m_bgCallback();
    } else {
        SDL_SetRenderDrawColor(rend, 2, 5, 15, 255);
        SDL_RenderClear(rend);
    }

    // Overlay semitransparente sobre el fondo
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, 0, 0, 0, 180);
    SDL_Rect full{ 0, 0, W, H };
    SDL_RenderFillRect(rend, &full);

    // Título
    BitmapFont::drawTextCentered(rend, "DIFICULTAD", CX, 90, 4, {0, 220, 255, 255});
    BitmapFont::drawTextCentered(rend, "SELECCIONA EL NIVEL DE DESAFIO",
                                 CX, 148, 1, {120, 160, 190, 200});

    // Opciones
    for (int i = 0; i < 3; i++)
        drawOption(m_buttons[i], (Difficulty)i, i == m_selected);

    // Descripción de la opción seleccionada
    BitmapFont::drawTextCentered(rend, description((Difficulty)m_selected),
                                 CX, OPT_Y + OPT_H + 32, 1, {160, 200, 220, 220});

    // Controles
    BitmapFont::drawTextCentered(rend, "FLECHAS   ENTER   ESC VOLVER",
                                 CX, H - 30, 1, {60, 80, 100, 180});

    SDL_RenderPresent(rend);
}

/* Dibuja una opción con icono visual propio para cada dificultad. */
void DifficultyScreen::drawOption(const Button& btn, Difficulty diff,
                                  bool selected) const {
    SDL_Renderer* rend = m_renderer.sdlRenderer();
    const SDL_Rect& r  = btn.rect;

    // Color de acento por dificultad
    SDL_Color accent;
    switch (diff) {
        case Difficulty::EASY:   accent = {  60, 200, 100, 255}; break;
        case Difficulty::HARD:   accent = { 220,  60,  60, 255}; break;
        default:                 accent = {  60, 160, 255, 255}; break;
    }

    // Fondo del panel
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_Color bg = selected ? SDL_Color{(Uint8)(accent.r/4), (Uint8)(accent.g/4), (Uint8)(accent.b/4), 220}
                            : SDL_Color{10, 15, 35, 180};
    SDL_SetRenderDrawColor(rend, bg.r, bg.g, bg.b, bg.a);
    SDL_RenderFillRect(rend, &r);

    // Borde — doble si seleccionado
    SDL_SetRenderDrawColor(rend, accent.r, accent.g, accent.b,
                           selected ? 255 : 120);
    SDL_RenderDrawRect(rend, &r);
    if (selected) {
        SDL_Rect inner{ r.x+1, r.y+1, r.w-2, r.h-2 };
        SDL_RenderDrawRect(rend, &inner);
    }

    // Etiqueta
    const int labelScale = 2;
    const int labelY     = r.y + 20;
    BitmapFont::drawTextCentered(rend, btn.label.c_str(),
                                 r.x + r.w/2, labelY, labelScale,
                                 selected ? accent : SDL_Color{160, 180, 200, 200});

    // Icono de estrellas de dificultad (1, 2 o 3 puntos)
    const int stars  = (int)diff + 1;
    const int starSz = 8;
    const int starY  = r.y + 55;
    const int totalStarW = stars * starSz + (stars - 1) * 6;
    int sx = r.x + r.w/2 - totalStarW/2;
    for (int s = 0; s < stars; s++) {
        SDL_Rect star{ sx, starY, starSz, starSz };
        SDL_SetRenderDrawColor(rend, accent.r, accent.g, accent.b,
                               selected ? 255 : 140);
        SDL_RenderFillRect(rend, &star);
        sx += starSz + 6;
    }

    // Multiplicador de velocidad como referencia numérica
    const char* mult[] = { "x0.65", "x1.00", "x1.45" };
    BitmapFont::drawTextCentered(rend, mult[(int)diff],
                                 r.x + r.w/2, r.y + OPT_H - 22, 1,
                                 selected ? accent : SDL_Color{100, 120, 140, 180});
}