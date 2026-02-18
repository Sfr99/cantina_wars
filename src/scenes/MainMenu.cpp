#include "MainMenu.hpp"
#include "../audio/music.hpp"
#include "../ui/BitmapFont.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>

static constexpr int W = 900;
static constexpr int H = 600;
static constexpr int CX = W / 2;

// Layout de botones
static constexpr int BTN_W      = 280;
static constexpr int BTN_H      = 48;
static constexpr int BTN_GAP    = 16;
static constexpr int BTN_START_Y = 230;

MainMenu::MainMenu(audio::MusicSystem& audio, Renderer& renderer)
    : m_audio(audio), renderer(renderer)
{
    buildButtons();
    initBackground();
}

MainMenu::~MainMenu() {}

/* Crea los cinco botones centrados; solo JUGAR y SALIR están habilitados. */
void MainMenu::buildButtons() {
    struct Def { const char* label; bool enabled; };
    const Def defs[] = {
        {"JUGAR",          true },
        {"DIFICULTAD",     false},
        {"RANKING",        false},
        {"CONFIGURACION",  false},
        {"SALIR",          true },
    };

    int y = BTN_START_Y;
    for (auto& d : defs) {
        Button btn;
        btn.label   = d.label;
        btn.enabled = d.enabled;
        btn.state   = d.enabled ? ButtonState::IDLE : ButtonState::DISABLED;
        btn.rect    = { CX - BTN_W / 2, y, BTN_W, BTN_H };
        m_buttons.push_back(btn);
        y += BTN_H + BTN_GAP;
    }

    // El primer seleccionable por teclado es JUGAR
    m_selected = 0;
}

/* Inicializa las tres capas de estrellas y los asteroides de fondo. */
void MainMenu::initBackground() {
    // Tres capas con velocidades y tamaños distintos (lejos=lento=pequeño)
    const float speeds[]  = { 18.f, 45.f, 95.f };
    const int   sizes[]   = { 1, 1, 2 };
    const int   counts[]  = { 80, 50, 30 };

    for (int l = 0; l < 3; l++) {
        m_layers[l].speed   = speeds[l];
        m_layers[l].dotSize = sizes[l];
        for (int i = 0; i < counts[l]; i++) {
            StarLayer::Star s;
            s.x          = (float)(rand() % W);
            s.y          = (float)(rand() % H);
            s.brightness = (Uint8)(80 + rand() % 176);
            m_layers[l].stars.push_back(s);
        }
    }

    // Cámara fija mirando hacia adelante
    camera.eye    = { 0.f, 2.f, -18.f };
    camera.target = { 0.f, 0.f,  30.f };

    // Asteroides flotantes lentos en LOW_POLY
    const Vec3 positions[] = {
        {-18.f,  3.f, 20.f}, { 14.f, -4.f, 25.f},
        { -6.f, -6.f, 35.f}, { 20.f,  6.f, 15.f},
        {  2.f,  8.f, 40.f}, {-22.f, -2.f, 30.f},
    };
    const AsteroidSize sizes2[] = {
        AsteroidSize::LARGE, AsteroidSize::MEDIUM,
        AsteroidSize::SMALL, AsteroidSize::LARGE,
        AsteroidSize::MEDIUM, AsteroidSize::SMALL,
    };

    for (int i = 0; i < 6; i++) {
        Vec3 vel = {
            ((rand() % 60) - 30) * 0.01f,
            ((rand() % 40) - 20) * 0.01f,
            -1.5f - (rand() % 20) * 0.05f
        };
        m_bgAsteroids.emplace_back(positions[i], vel, sizes2[i],
                                   RenderMode::LOW_POLY, i * 17 + 3);
    }
}

MenuResult MainMenu::run() {


    Uint32 lastTime = SDL_GetTicks();

    while (!m_done) {
        Uint32 now = SDL_GetTicks();
        float  dt  = std::min((now - lastTime) / 1000.f, 0.05f);
        lastTime   = now;

        handleEvents();
        update(dt);
        render();
    }

    return m_result;
}

/* Avanza el índice seleccionado por teclado saltando botones deshabilitados. */
static int nextEnabled(const std::vector<Button>& btns, int current, int dir) {
    int n = (int)btns.size();
    int idx = current;
    for (int i = 0; i < n; i++) {
        idx = (idx + dir + n) % n;
        if (btns[idx].enabled) return idx;
    }
    return current;
}

/* Procesa eventos SDL: cierre, ESC, flechas, Enter y movimiento de ratón. */
void MainMenu::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            m_result = MenuResult::QUIT;
            m_done   = true;
            return;
        }

        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    m_result = MenuResult::QUIT;
                    m_done   = true;
                    break;
                case SDLK_DOWN:
                    m_selected = nextEnabled(m_buttons, m_selected, +1);
                    break;
                case SDLK_UP:
                    m_selected = nextEnabled(m_buttons, m_selected, -1);
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (m_buttons[m_selected].label == "JUGAR") {
                        m_result = MenuResult::PLAY;
                        m_done   = true;
                    } else if (m_buttons[m_selected].label == "SALIR") {
                        m_result = MenuResult::QUIT;
                        m_done   = true;
                    }
                    break;
            }
        }

        if (e.type == SDL_MOUSEMOTION) {
            for (int i = 0; i < (int)m_buttons.size(); i++) {
                auto& b = m_buttons[i];
                if (!b.enabled) continue;
                if (b.contains(e.motion.x, e.motion.y)) {
                    b.state    = ButtonState::HOVER;
                    m_selected = i;
                } else {
                    b.state = ButtonState::IDLE;
                }
            }
        }

        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            for (auto& b : m_buttons) {
                if (!b.enabled) continue;
                if (!b.contains(e.button.x, e.button.y)) continue;
                if (b.label == "JUGAR") { m_result = MenuResult::PLAY;  m_done = true; }
                if (b.label == "SALIR") { m_result = MenuResult::QUIT;  m_done = true; }
            }
        }
    }
}

/* Actualiza fondo dinámico. */
void MainMenu::update(float dt) {
    updateStars(dt);
    updateBgAsteroids(dt);
}

/* Desplaza cada capa de estrellas hacia abajo; las que salen por abajo reaparecen arriba. */
void MainMenu::updateStars(float dt) {
    for (auto& layer : m_layers) {
        for (auto& s : layer.stars) {
            s.y += layer.speed * dt;
            if (s.y > H) {
                s.y = -2.f;
                s.x = (float)(rand() % W);
            }
        }
    }
}

/* Mueve los asteroides y los recicla cuando se alejan demasiado de la cámara. */
void MainMenu::updateBgAsteroids(float dt) {
    for (auto& a : m_bgAsteroids) {
        a.update(dt);
        // Reciclar si sale del volumen visible
        if (a.pos.z < camera.eye.z + 2.f) {
            a.pos.z = camera.eye.z + 45.f + (rand() % 20);
            a.pos.x = ((rand() % 50) - 25) * 1.f;
            a.pos.y = ((rand() % 30) - 15) * 1.f;
        }
    }
}

/* Renderiza el frame completo del menú. */
void MainMenu::render() {
    // Fondo negro base
    SDL_SetRenderDrawColor(renderer.sdlRenderer(), 2, 5, 15, 255);
    SDL_RenderClear(renderer.sdlRenderer());

    drawStarLayers();
    drawBgAsteroids();
    drawUI();

    SDL_RenderPresent(renderer.sdlRenderer());
}

/* Dibuja las tres capas de estrellas con opacidad proporcional a su velocidad. */
void MainMenu::drawStarLayers() {
    SDL_Renderer* rend = renderer.sdlRenderer();
    const Uint8 alphas[] = { 140, 180, 255 };

    for (int l = 0; l < 3; l++) {
        const auto& layer = m_layers[l];
        for (const auto& s : layer.stars) {
            SDL_SetRenderDrawColor(rend, s.brightness, s.brightness, s.brightness, alphas[l]);
            SDL_Rect px{ (int)s.x, (int)s.y, layer.dotSize, layer.dotSize };
            SDL_RenderFillRect(rend, &px);
        }
    }
}

/* Dibuja los asteroides LOW_POLY del fondo con un tono apagado para no competir con la UI. */
void MainMenu::drawBgAsteroids() {
    Mat4 view = camera.viewMatrix();
    for (const auto& a : m_bgAsteroids) {
        SDL_Color col = (a.size == AsteroidSize::LARGE)  ? SDL_Color{ 60,  45, 30, 255}
                      : (a.size == AsteroidSize::MEDIUM) ? SDL_Color{ 50,  55, 35, 255}
                                                         : SDL_Color{ 45,  60, 40, 255};
        renderer.drawMesh(a.getMesh(), a.worldTransform(), view, col);
    }
}

/* Dibuja el título y los botones del menú. */
void MainMenu::drawUI() {
    SDL_Renderer* rend = renderer.sdlRenderer();

    // Título con sombra (dibuja dos veces: sombra desplazada + texto principal)
    const int TITLE_SCALE = 5;
    const int TITLE_Y     = 80;
    BitmapFont::drawTextCentered(rend, "CANTINA WARS", CX + 2, TITLE_Y + 2,
                                 TITLE_SCALE, {0, 80, 120, 180});
    BitmapFont::drawTextCentered(rend, "CANTINA WARS", CX, TITLE_Y,
                                 TITLE_SCALE, {0, 220, 255, 255});

    // Botones
    for (int i = 0; i < (int)m_buttons.size(); i++)
        drawButton(m_buttons[i], i == m_selected);

    // Leyenda de controles al pie
    BitmapFont::drawTextCentered(rend, "ARROWS   ENTER   ESC", CX, H - 30,
                                 1, {60, 80, 100, 180});
}

/* Dibuja un botón con fondo, borde y etiqueta según su estado. */
void MainMenu::drawButton(const Button& btn, bool selected) {
    SDL_Renderer* rend = renderer.sdlRenderer();
    const SDL_Rect& r  = btn.rect;

    // Colores según estado
    SDL_Color bgCol, borderCol, textCol;
    if (!btn.enabled) {
        bgCol     = {10,  15,  30,  120};
        borderCol = {50,  55,  70,  140};
        textCol   = {70,  75,  90,  180};
    } else if (selected) {
        bgCol     = {10,  60, 120,  210};
        borderCol = {0,  220, 255,  255};
        textCol   = {0,  240, 255,  255};
    } else {
        bgCol     = {10,  20,  50,  180};
        borderCol = {0,  120, 180,  200};
        textCol   = {180, 210, 230, 255};
    }

    // Fondo
    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(rend, bgCol.r, bgCol.g, bgCol.b, bgCol.a);
    SDL_RenderFillRect(rend, &r);

    // Borde (dibuja los 4 lados como rects de 1px)
    SDL_SetRenderDrawColor(rend, borderCol.r, borderCol.g, borderCol.b, borderCol.a);
    SDL_RenderDrawRect(rend, &r);
    if (selected) {
        // Borde doble para el seleccionado
        SDL_Rect inner{ r.x + 1, r.y + 1, r.w - 2, r.h - 2 };
        SDL_RenderDrawRect(rend, &inner);
    }

    // Etiqueta centrada verticalmente en el botón
    const int TEXT_SCALE = 2;
    const int textH      = BitmapFont::GLYPH_H * TEXT_SCALE;
    const int textY      = r.y + (r.h - textH) / 2;

    // Sombra del texto para botones activos
    if (btn.enabled) {
        BitmapFont::drawTextCentered(rend, btn.label.c_str(), CX + 1, textY + 1,
                                     TEXT_SCALE, {0, 0, 0, 120});
    }
    BitmapFont::drawTextCentered(rend, btn.label.c_str(), CX, textY,
                                 TEXT_SCALE, textCol);

    // Etiqueta "PROXIMAMENTE" para botones deshabilitados
    if (!btn.enabled) {
        const int tagX = r.x + r.w - 90;
        const int tagY = r.y + (r.h - BitmapFont::GLYPH_H) / 2;
        BitmapFont::drawText(rend, "PROX", tagX, tagY, 1, {60, 65, 80, 180});
    }
}