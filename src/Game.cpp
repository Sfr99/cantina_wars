#include "Game.hpp"
#include "entities/Spawner.hpp"
#include "audio/music.hpp"
#include <SDL2/SDL_mixer.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>

Game::Game(audio::MusicSystem& audio) : m_audio(audio), renderer(900, 600) {
    srand((unsigned)time(nullptr));

    shipMeshHD = GLTFLoader::loadShipMesh("../assets/ship/scene.bin");
    shipMesh   = Ship::createMesh();
    bulletMesh = Bullet::createMesh();
    Bullet::laserTexture = Bullet::createLaserTexture();

    m_audio.init();
    m_audio.preloadSFX("../assets/sounds/laser.mp3");
    m_audio.preloadSFX("../assets/sounds/explosion.wav");
    m_audio.preloadSFX("../assets/sounds/boost.mp3");

    for (int i = 0; i < 15; i++) spawnAsteroid();
}

/* Carga música, lanza la primera oleada y ejecuta el loop hasta que running=false. */
void Game::run() {
    if (!renderer.init()) return;

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
        m_hyperIntensity = hyperspace.charge;

        // Gestiona el canal de audio del boost (loop mientras activo)
        const bool isBoosting = hyperspace.charge > 0.1f;
        static bool wasBoosting = false;
        if (isBoosting && !wasBoosting)
            m_boostChannel = m_audio.playSFX("../assets/sounds/boost.mp3", -1);
        else if (!isBoosting && wasBoosting && m_boostChannel >= 0) {
            Mix_HaltChannel(m_boostChannel);
            m_boostChannel = -1;
        }
        wasBoosting = isBoosting;

        update(dt);
        render();
    }
}

/* Procesa eventos SDL: cierre de ventana, ESC, R (restart), T (toggle render mode). */
void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) { running = false; return; }
        if (e.type != SDL_KEYDOWN) continue;

        switch (e.key.keysym.sym) {
            case SDLK_ESCAPE:
                running = false;
                break;

            case SDLK_r:
                if (state == GameState::GAME_OVER) restart();
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

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    ship.setSpeedMultiplier(hyperspace.speedMultiplier());
    ship.handleInput(keys, dt);
    ship.update(dt);

    if (ship.wantsShoot) {
        constexpr float MUZZLE_SPEED = 90.f;
        Vec3 bpos = ship.pos + Vec3{0.f, 0.f, 2.5f};
        Vec3 bvel = {0.f, 0.f, ship.vel.z + MUZZLE_SPEED};
        bullets.emplace_back(bpos, bvel);
        m_audio.playSFX("../assets/sounds/laser.mp3");
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
        float difficulty = std::min(1.f + (score / 2500.f), 2.5f);
        spawnTimer = (0.28f / difficulty) + (rand() / (float)RAND_MAX) * 0.12f;
    }

    score += (int)(ship.vel.z * dt * 0.5f);
    updateHUD();
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
            m_audio.playSFX("../assets/sounds/explosion.wav");
            lives--;

            if (lives <= 0) {
                state = GameState::GAME_OVER;
                renderer.setWindowTitle("GAME OVER | Score: " + std::to_string(score)
                                        + " | Pulsa R para reiniciar");
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
    Spawner::spawnAsteroids(asteroids, ship.pos, renderMode, wave);
}

/* Rasteriza nave, asteroides, balas y efectos de velocidad. */
void Game::render() {
    renderer.clear(renderMode == RenderMode::HD);

    camera.follow(ship.pos, ship.fwd, ship.up);
    Mat4 view = camera.viewMatrix();

    // La nave parpadea mientras es invencible
    const bool drawShip = !ship.invincible || ((SDL_GetTicks() / 120) % 2 == 0);
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

    renderer.updateSpeedLines(m_dt, m_hyperIntensity);
    renderer.drawSpeedLines(m_hyperIntensity);
    renderer.present();
}

/* Actualiza el título de la ventana con puntuación, vidas y modo actual. */
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
    for (int i = 0; i < 15; i++) spawnAsteroid();
}