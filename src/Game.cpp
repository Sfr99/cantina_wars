#include "Game.hpp"
#include "scenes/RankingScreen.hpp"
#include "ui/BitmapFont.hpp"
#include "entities/Spawner.hpp"
#include "audio/music.hpp"
#include <SDL2/SDL_mixer.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>

Game::Game(audio::MusicSystem& audio, Renderer& renderer, const GameConfig& config,
             RankingSystem& ranking)
    : m_audio(audio), renderer(renderer), m_config(config), m_ranking(ranking),
      m_hud(renderer.width(), renderer.height()),
      m_overlay(renderer.width(), renderer.height()) {
    srand((unsigned)time(nullptr));

    // Guardar top score actual para detectar superación durante la partida
    m_prevTopScore = m_ranking.entries().empty() ? 0 : m_ranking.entries()[0].score;

    // Aplicar parámetros de dificultad
    auto params = m_config.params();
    hyperspace.chargeTime = params.boostChargeTime;

    shipMeshHD = GLTFLoader::loadShipMesh("../assets/ship/scene.bin");
    shipMesh   = Ship::createMesh();
    bulletMesh = Bullet::createMesh();
    Bullet::laserTexture = Bullet::createLaserTexture();

    m_audio.init();
    m_audio.preloadSFX("../assets/sounds/laser.ogg");
    m_audio.preloadSFX("../assets/sounds/explosion.ogg");
    m_audio.preloadSFX("../assets/sounds/boost.ogg");
    m_audio.preloadSFX("../assets/sounds/new_high_score.wav");
    m_audio.preloadSFX("../assets/sounds/victory.wav");
    m_audio.preloadSFX("../assets/sounds/lose.wav");

    for (int i = 0; i < params.baseAsteroidCount; i++) spawnAsteroid();
}

/* Carga música, lanza la primera oleada y ejecuta el loop hasta que running=false. */
void Game::run() {


    m_audio.loadMusic("../assets/sounds/cantina.mp3");
    m_audio.playMusic(-1);
    spawnWave();

    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = std::min((now - lastTime) / 1000.f, 0.05f);
        lastTime   = now;

        handleEvents();

        const Uint8* ks        = SDL_GetKeyboardState(nullptr);
        const bool   boostHeld = ks[SDL_SCANCODE_SPACE];

        hyperspace.update(boostHeld, dt);
        m_dt             = dt;
        m_hyperIntensity = hyperspace.visualIntensity();

        // Audio del boost: arranca al pulsar SPACE con carga, para al soltar o al vaciarse
        if (hyperspace.enteredFiring)
            m_boostChannel = m_audio.playSFX("../assets/sounds/boost.ogg", 0);
        if (hyperspace.leftFiring && m_boostChannel >= 0) {
            Mix_HaltChannel(m_boostChannel);
            m_boostChannel = -1;
        }

        update(dt);
        render();
    }
}

/* Procesa eventos SDL: cierre de ventana, ESC, R (restart), T (toggle render mode). */
void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running = false; return; }
        if (state == GameState::NAME_ENTRY) { handleNameEntryEvent(e); continue; }
        if (e.type != SDL_KEYDOWN) continue;

        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                running = false;
                break;

            case SDLK_r:
                if (state == GameState::GAME_OVER) restart();
                break;

            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                if (state == GameState::NAME_ENTRY) submitScore();
                break;

            case SDLK_t:
                if (state != GameState::PLAYING) break;
                // Alterna render mode y regenera todos los asteroides
                renderMode = (renderMode == RenderMode::LOW_POLY)
                           ? RenderMode::HD : RenderMode::LOW_POLY;
                {
                    struct AsteroidSnapshot { Vec3 pos, vel; AsteroidSize size; };
                    std::vector<AsteroidSnapshot> snap;
                    snap.reserve(asteroids.size());
                    for (auto& a : asteroids)
                        snap.push_back({a.pos, a.vel, a.size});

                    asteroids.clear();
                    for (auto& s : snap)
                        asteroids.emplace_back(s.pos, s.vel, s.size, renderMode, rand());
                }
                break;
        }
    }
}

/* Actualiza input, física, spawn continuo y puntaje por distancia. */
void Game::update(float dt) {
    if (state == GameState::GAME_OVER) return;

    // En NAME_ENTRY solo avanzan los asteroides; la nave desaparece y no hay colisiones
    if (state == GameState::NAME_ENTRY) {
        for (auto& a : asteroids) a.update(dt);
        asteroids.erase(
            std::remove_if(asteroids.begin(), asteroids.end(),
                [this](const Asteroid& a) { return a.pos.z < ship.pos.z - 30.f; }),
            asteroids.end());
        return;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    ship.setSpeedMultiplier(hyperspace.speedMultiplier());
    ship.handleInput(keys, dt);
    ship.update(dt);

    if (ship.wantsShoot) {
        constexpr float MUZZLE_SPEED = 90.f;
        Vec3 bpos = ship.pos + Vec3{0.f, 0.f, 2.5f};
        Vec3 bvel = {0.f, 0.f, ship.vel.z + MUZZLE_SPEED};
        bullets.emplace_back(bpos, bvel);
        m_audio.playSFX("../assets/sounds/laser.ogg");
    }

    for (auto& a : asteroids) a.update(dt);
    for (auto& b : bullets)   b.update(dt);

    // Eliminar balas expiradas o muy adelantadas
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [this](const Bullet& b) {
                return b.expired() || b.pos.z > ship.pos.z + 100.f;
            }),
        bullets.end());

    // Eliminar asteroides que quedaron muy atrás
    asteroids.erase(
        std::remove_if(asteroids.begin(), asteroids.end(),
            [this](const Asteroid& a) {
                return !a.alive || a.pos.z < ship.pos.z - 30.f;
            }),
        asteroids.end());

    checkCollisions();

    // Mantener un mínimo de asteroides por oleada
    int target = 25 + wave * 3;
    while ((int)asteroids.size() < target) spawnAsteroid();

    // Spawn periódico (frecuencia aumenta con el score)
    spawnTimer -= dt;
    if (spawnTimer <= 0.f) {
        spawnAsteroid();
        float progression = std::min(1.f + (score / 2500.f), 2.5f);
        float spawnMult = progression * m_config.params().spawnRateMult;
        spawnTimer = (0.28f / spawnMult) + (rand() / (float)RAND_MAX) * 0.12f;
    }

    score += (int)(ship.vel.z * dt * 0.5f);

    // Detectar nuevo high score durante la partida
    if (!m_newHighScore && score > m_prevTopScore && score > 0) {
        m_newHighScore     = true;
        m_highScoreBannerT = 4.0f;
        m_audio.playSFX("../assets/sounds/new_high_score.wav");
    }
    if (m_highScoreBannerT > 0.f) m_highScoreBannerT -= dt;

}

bool Game::sphereCollide(Vec3 a, float ra, Vec3 b, float rb) {
    Vec3  d       = a - b;
    float minDist = ra + rb;
    return d.dot(d) < minDist * minDist;
}

/* Detecta colisiones bala-asteroide y nave-asteroide; aplica daño y puntaje. */
void Game::checkCollisions() {
    for (auto& b : bullets) {
        if (!b.alive) continue;
        for (auto& a : asteroids) {
            if (!a.alive) continue;
            if (!sphereCollide(b.pos, Bullet::RADIUS, a.pos, a.radius)) continue;

            b.alive = false;
            a.alive = false;

            switch (a.size) {
                case AsteroidSize::LARGE:  score += 20;  break;
                case AsteroidSize::MEDIUM: score += 50;  break;
                case AsteroidSize::SMALL:  score += 100; break;
            }
        }
    }

    if (!ship.invincible && ship.alive) {
        for (auto& a : asteroids) {
            if (!a.alive) continue;
            if (!sphereCollide(ship.pos, 1.5f, a.pos, a.radius)) continue;

            a.alive = false;
            m_audio.playSFX("../assets/sounds/explosion.ogg");
            lives--;

            if (lives <= 0) {
                enterGameOver();
            } else {
                ship.respawn();
            }
            break;
        }
    }

    auto isDead = [](const auto& e) { return !e.alive; };
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), isDead), bullets.end());
    asteroids.erase(std::remove_if(asteroids.begin(), asteroids.end(), isDead), asteroids.end());
}

/* Lanza una oleada numerada con 5 + wave*3 asteroides adicionales. */
void Game::spawnWave() {
    wave++;
    int count = 5 + wave * 3;
    for (int i = 0; i < count; i++) spawnAsteroid();
}

/* Delega la generación de asteroides al módulo Spawner. */
void Game::spawnAsteroid() {
    Spawner::spawnAsteroids(asteroids, ship.pos, renderMode, wave, m_config.params());
}

/* Rasteriza nave, asteroides, balas y efectos de velocidad. */
void Game::render() {
    renderer.clear(renderMode == RenderMode::HD);

    camera.follow(ship.pos, ship.fwd, ship.up);
    Mat4 view = camera.viewMatrix();

    // La nave parpadea mientras es invencible; no se dibuja en NAME_ENTRY
    const bool drawShip = state != GameState::NAME_ENTRY &&
                          (!ship.invincible || ((SDL_GetTicks() / 120) % 2 == 0));
    if (ship.alive && drawShip) {
        if (renderMode == RenderMode::HD) {
            Mat4 hdTransform = ship.worldTransform() * Mat4::rotationX(-1.5708f);
            renderer.drawFilledMesh(shipMeshHD, hdTransform, view, nullptr);
        } else {
            renderer.drawMesh(shipMesh, ship.worldTransform(), view, {0, 210, 255, 255});
        }
    }

    for (const auto& a : asteroids) {
        if (renderMode == RenderMode::HD) {
            renderer.drawFilledMesh(a.getMesh(), a.worldTransform(), view, a.diffuseTexture);
        } else {
            SDL_Color col = (a.size == AsteroidSize::LARGE)  ? SDL_Color{200, 120,  30, 255}
                          : (a.size == AsteroidSize::MEDIUM) ? SDL_Color{180, 160,  60, 255}
                                                             : SDL_Color{160, 200,  90, 255};
            renderer.drawMesh(a.getMesh(), a.worldTransform(), view, col);
        }
    }

    for (const auto& b : bullets) {
        if (renderMode == RenderMode::HD)
            renderer.drawFilledMesh(bulletMesh, b.worldTransform(), view, Bullet::laserTexture);
        else
            renderer.drawMesh(bulletMesh, b.worldTransform(), view, {255, 255, 50, 255});
    }

    // Subir pixel buffer a GPU de una sola vez (todos los meshes rellenos ya escritos)
    renderer.flushPixelBuffer();

    renderer.updateSpeedLines(m_dt, m_hyperIntensity);
    renderer.drawSpeedLines(m_hyperIntensity);

    // HUD siempre visible durante la partida
    m_hud.draw(renderer.sdlRenderer(), score, lives,
               renderMode == RenderMode::HD, hyperspace);

    // Overlays según estado
    if (state == GameState::GAME_OVER) {
        std::string sub = "SCORE  " + std::to_string(score);
        m_overlay.draw(renderer.sdlRenderer(),
                       "GAME OVER", sub.c_str(), "PULSA R PARA REINICIAR");
    } else if (state == GameState::NAME_ENTRY) {
        std::string sub = "SCORE  " + std::to_string(score);
        m_overlay.drawNameEntry(renderer.sdlRenderer(),
                                "NUEVO RECORD", sub.c_str(), m_nameEntry, m_nameCursor);
    }

    // Banner de new high score durante el juego
    if (m_highScoreBannerT > 0.f && state == GameState::PLAYING)
        drawHighScoreBanner();

    renderer.present();
}

/* Actualiza el título de la ventana con puntuación, vidas y modo actual. NO USAR, SOLO DEBUG */
void Game::updateHUD() {
    const char* mode = (renderMode == RenderMode::HD) ? "HD" : "LOW_POLY";
    renderer.setWindowTitle(
        "Asteroids 3D  |  Score: " + std::to_string(score) +
        "  |  Lives: "             + std::to_string(lives) +
        "  |  Mode: "              + std::string(mode) +
        "  |  [WASD/Arrows] move  [F] fire  [T] toggle HD"
    );
}

/* Reinicia el estado completo del juego manteniendo la sesión activa. */
void Game::restart() {
    score      = 0;
    lives      = 3;
    wave       = 0;
    state      = GameState::PLAYING;
    spawnTimer = 0.5f;
    asteroids.clear();
    bullets.clear();
    ship.respawn();
    for (int i = 0; i < m_config.params().baseAsteroidCount; i++) spawnAsteroid();
}

/* Transiciona a GAME_OVER o NAME_ENTRY según si el score entra en el ranking. */
void Game::enterGameOver() {
    if (m_ranking.isHighScore(score)) {
        state          = GameState::NAME_ENTRY;
        m_nameCursor   = 0;
        m_nameEntry[0] = m_nameEntry[1] = m_nameEntry[2] = 'A';
        m_audio.playSFX("../assets/sounds/victory.wav");
    } else {
        state = GameState::GAME_OVER;
        m_audio.playSFX("../assets/sounds/explosion.ogg");
        m_audio.playSFX("../assets/sounds/lose.wav");
    }
    renderer.setWindowTitle("GAME OVER | Score: " + std::to_string(score));
}

/* Procesa eventos de teclado durante la entrada de nombre. */
void Game::handleNameEntryEvent(const SDL_Event& e) {
    if (e.type != SDL_KEYDOWN) return;
    switch (e.key.keysym.sym) {
        case SDLK_UP:
            m_nameEntry[m_nameCursor] =
                (m_nameEntry[m_nameCursor] == 'A') ? 'Z'
                : (char)(m_nameEntry[m_nameCursor] - 1);
            break;
        case SDLK_DOWN:
            m_nameEntry[m_nameCursor] =
                (m_nameEntry[m_nameCursor] == 'Z') ? 'A'
                : (char)(m_nameEntry[m_nameCursor] + 1);
            break;
        case SDLK_LEFT:
            if (m_nameCursor > 0) m_nameCursor--;
            break;
        case SDLK_RIGHT:
            if (m_nameCursor < 2) m_nameCursor++;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            submitScore();
            break;
        case SDLK_ESCAPE:
            // Cancelar entrada — guardar con nombre por defecto
            m_nameEntry[0] = m_nameEntry[1] = m_nameEntry[2] = '?';
            submitScore();
            break;
    }
}

/* Guarda el score, muestra el ranking y transiciona a GAME_OVER. */
void Game::submitScore() {
    m_ranking.insertScore(m_nameEntry, score);

    // Mostrar pantalla de ranking inmediatamente después de confirmar
    RankingScreen rs(renderer, m_ranking, nullptr, score);
    rs.run();

    state = GameState::GAME_OVER;
}

/* Dibuja el banner de nuevo high score con fade-out en los últimos segundos. */
void Game::drawHighScoreBanner() {
    SDL_Renderer* rend  = renderer.sdlRenderer();
    const float   total = 4.0f;
    // Fade out en el último segundo
    float alpha = (m_highScoreBannerT < 1.0f) ? m_highScoreBannerT : 1.0f;
    Uint8 a = (Uint8)(alpha * 255);

    SDL_SetRenderDrawBlendMode(rend, SDL_BLENDMODE_BLEND);

    // Fondo del banner
    SDL_Rect bg{ renderer.width()/2 - 200, 20, 400, 36 };
    SDL_SetRenderDrawColor(rend, 0, 30, 60, (Uint8)(alpha * 180));
    SDL_RenderFillRect(rend, &bg);
    SDL_SetRenderDrawColor(rend, 0, 200, 255, a);
    SDL_RenderDrawRect(rend, &bg);

    // Texto parpadeante
    bool blink = (SDL_GetTicks() / 200) % 2 == 0;
    if (blink || m_highScoreBannerT > 2.0f)
        BitmapFont::drawTextCentered(rend, "NUEVO RECORD",
                                     renderer.width()/2, 28, 2,
                                     {0, (Uint8)(200 * alpha), (Uint8)(255 * alpha), a});
}