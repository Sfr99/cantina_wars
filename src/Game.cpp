#include "Game.hpp"
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>
#include <algorithm>
#include <random>
#include "music.hpp"

static float approach01(float v, float target, float dt, float timeToTarget) {
    if (timeToTarget <= 1e-6f) return target;
    float step = dt / timeToTarget;
    if (v < target) { v += step; if (v > target) v = target; }
    else { v -= step; if (v < target) v = target; }
    return v;
}

Game::Game(audio::MusicSystem& audio) : m_audio(audio), renderer(900, 600) {
    srand((unsigned)time(nullptr));
    shipMeshHD  = Ship::loadGLTFMesh("../assets/ship/scene.bin");
    shipMesh   = Ship::createMesh();

    bulletMesh = Bullet::createMesh();
    m_audio.init();

    m_audio.loadMusic("../assets/sounds/cantina.mp3");
    m_audio.playMusic(-1); 
    m_audio.preloadSFX("../assets/sounds/laser.mp3");

    
    // Spawn inicial de asteroides
   // printf("Spawning initial asteroids...\n");
    for (int i = 0; i < 15; i++) {
        spawnAsteroid();
    }
    // printf("Initial asteroids: %zu\n", asteroids.size());
}

void Game::spawnWave() {
    wave++;
    int count = 5 + wave * 3; 
   //printf("\n=== SPAWNING WAVE %d: %d ASTEROIDS ===\n", wave, count);
    for (int i = 0; i < count; i++) {
        spawnAsteroid();
    }
}


void Game::run() {
    if (!renderer.init()) return;

    spawnWave();

    Uint32 lastTime = SDL_GetTicks();

    while (running) {
        Uint32 now = SDL_GetTicks();
        float  dt  = (now - lastTime) / 1000.f;
        lastTime   = now;
        if (dt > 0.05f) dt = 0.05f;  // cap a 20fps mínimo

        handleEvents();
        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        const bool spaceHeld = ks[SDL_SCANCODE_SPACE];
        static Uint32 t0 = SDL_GetTicks();
        /*if (SDL_GetTicks() - t0 < 5000) {
            printf("spaceHeld=%d charge=%.2f speedMult=%.2f\n",
                spaceHeld ? 1 : 0, hyperspace.charge, 1.f + hyperspace.charge * (hyperspace.maxMult - 1.f));
        }*/

        hyperspace.charge = approach01(
            hyperspace.charge,
            spaceHeld ? 1.f : 0.f,
            dt,
            spaceHeld ? hyperspace.chargeTime : hyperspace.decayTime
        );

        m_dt = dt;
        m_hyperIntensity = hyperspace.charge;

        const float speedMult = 1.f + hyperspace.charge * (hyperspace.maxMult - 1.f);
        ship.setSpeedMultiplier(speedMult);

        update(dt);
        render();
    }
}

void Game::handleEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) running = false;
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) running = false;
            if (e.key.keysym.sym == SDLK_r && state == GameState::GAME_OVER) restart();
            if (e.key.keysym.sym == SDLK_t && state == GameState::PLAYING) {
                // Toggle render mode y regenerar asteroides
                renderMode = (renderMode == RenderMode::LOW_POLY) ? RenderMode::HD : RenderMode::LOW_POLY;
                
                const char* modeName = (renderMode == RenderMode::HD) ? "HD" : "LOW_POLY";
                printf("\n=== SWITCHING TO %s MODE ===\n", modeName);
                
                // Guardar estado de asteroides existentes
                struct AsteroidData { Vec3 pos, vel; AsteroidSize size; int seed; };
                std::vector<AsteroidData> data;
                for (auto& a : asteroids) {
                    data.push_back({a.pos, a.vel, a.size, rand()});
                }
                
                printf("Regenerating %zu asteroids...\n", data.size());
                
                // Regenerar con nuevo modo
                asteroids.clear();
                for (auto& d : data) {
                    asteroids.emplace_back(d.pos, d.vel, d.size, renderMode, d.seed);
                }
                
                printf("=== DONE ===\n\n");
            }
        }
    }
}



void Game::update(float dt) {
    if (state == GameState::GAME_OVER) return;

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    
    ship.handleInput(keys, dt);
    ship.update(dt);

    if (ship.wantsShoot) {
        constexpr float BULLET_MUZZLE_SPEED = 90.f;   // velocidad relativa extra (ajusta)
        Vec3 bpos = ship.pos + Vec3{0.f, 0.f, 2.5f};

        Vec3 bvel = {0.f, 0.f, ship.vel.z + BULLET_MUZZLE_SPEED};

        bullets.emplace_back(bpos, bvel);


        m_audio.playSFX("../assets/sounds/laser.mp3");
    }

    for (auto& a : asteroids) a.update(dt);
    for (auto& b : bullets)   b.update(dt);

    // Eliminar balas fuera de rango
    bullets.erase(
        std::remove_if(bullets.begin(), bullets.end(),
            [this](const Bullet& b){ 
                return b.expired() || b.pos.z > ship.pos.z + 100.f; 
            }),
        bullets.end());

    // Eliminar asteroides que pasaron por detrás
    asteroids.erase(
        std::remove_if(asteroids.begin(), asteroids.end(),
            [this](const Asteroid& a){ 
                return !a.alive || a.pos.z < ship.pos.z - 30.f; 
            }),
        asteroids.end());

    checkCollisions();
    
    int target = 25 + wave * 3;
    while ((int)asteroids.size() < target) spawnAsteroid();


    // Spawn continuo de asteroides adelante (MÁS FRECUENTE)
    spawnTimer -= dt;
    if (spawnTimer <= 0.f) {
        spawnAsteroid();
        // Spawn frecuente, aumenta con dificultad
        float difficulty = std::min(1.f + (score / 2500.f), 2.5f);
        spawnTimer = (0.28f / difficulty) + 
             (rand() / (float)RAND_MAX) * 0.12f;



    }
    
    // Puntos por distancia
    score += (int)(ship.vel.z * dt * 0.5f);

    updateHUD();
}

bool Game::sphereCollide(Vec3 a, float ra, Vec3 b, float rb) {
    Vec3 d = a - b;
    float minDist = ra + rb;
    return d.dot(d) < minDist * minDist;
}

void Game::checkCollisions() {
    // Balas vs asteroides
    for (auto& b : bullets) {
        if (!b.alive) continue;
        for (auto& a : asteroids) {
            if (!a.alive) continue;
            if (!sphereCollide(b.pos, Bullet::RADIUS, a.pos, a.radius)) continue;

            b.alive = false;
            a.alive = false;

            // Puntos según tamaño
            switch (a.size) {
                case AsteroidSize::LARGE:  score += 20;  break;
                case AsteroidSize::MEDIUM: score += 50;  break;
                case AsteroidSize::SMALL:  score += 100; break;
            }
        }
    }

    // Nave vs asteroides
    if (!ship.invincible && ship.alive) {
        for (auto& a : asteroids) {
            if (!a.alive) continue;
            if (sphereCollide(ship.pos, 1.5f, a.pos, a.radius)) {
                lives--;
                if (lives <= 0) {
                    state = GameState::GAME_OVER;
                    renderer.setWindowTitle("GAME OVER | Score: " + std::to_string(score) + " | Pulsa R para reiniciar");
                } else {
                    ship.respawn();
                }
                a.alive = false;  // destruir asteroide también
                break;
            }
        }
    }

    // Limpiar muertos
    auto isDead = [](const auto& e){ return !e.alive; };
    bullets.erase(std::remove_if(bullets.begin(), bullets.end(), isDead), bullets.end());
    asteroids.erase(std::remove_if(asteroids.begin(), asteroids.end(), isDead), asteroids.end());
}

static float frand01() { return rand() / (float)RAND_MAX; }
static float frand(float a, float b) { return a + (b - a) * frand01(); }

void Game::spawnAsteroid() {
    // Más lejos y con más "profundidad" para que haya más asteroides a la vez
    constexpr float MIN_SPAWN_Z_AHEAD = 180.f;
    constexpr float SPAWN_Z_RANGE     = 520.f;   // más rango => más población simultánea

    // Anillo en XY (evita el centro y reparte por toda la pantalla)
    constexpr float R_MIN = 10.f;   // radio mínimo del anillo (zona vacía central)
    constexpr float R_MAX = 38.f;   // radio máximo (amplitud del campo)

    // Zona segura delante de la nave
    constexpr float SAFE_RADIUS_XY = 5.0f;
    constexpr float SAFE_Z_NEAR    = 80.f;

    // Control de densidad por llamada (mejor 1-2 estable que 1-3 random)
    const int count = 2;

    for (int n = 0; n < count; ++n) {
        Vec3 spawnPos{};
        bool ok = false;

        for (int tries = 0; tries < 40 && !ok; ++tries) {
            float spawnZ = ship.pos.z + MIN_SPAWN_Z_AHEAD + frand(0.f, SPAWN_Z_RANGE);

            // Muestra uniforme en área de anillo: r = sqrt(u*(Rmax^2-Rmin^2)+Rmin^2)
            float u = frand01();
            float r = std::sqrt(u * (R_MAX*R_MAX - R_MIN*R_MIN) + R_MIN*R_MIN);
            float ang = frand(0.f, 6.28318530718f);

            float spawnX = r * std::cos(ang);
            float spawnY = r * std::sin(ang);

            spawnPos = {spawnX, spawnY, spawnZ};

            // 1) Zona segura cerca de la nave (cilindro + ventana en Z)
            Vec3 rel = spawnPos - ship.pos;
            float xy2 = rel.x*rel.x + rel.y*rel.y;
            if (std::fabs(rel.z) < SAFE_Z_NEAR && xy2 < SAFE_RADIUS_XY*SAFE_RADIUS_XY)
                continue;

            // 2) Selección de tamaño (igual que tú)
            AsteroidSize size;
            int rr = rand() % 100;
            if (rr < 60)      size = AsteroidSize::SMALL;
            else if (rr < 90) size = AsteroidSize::MEDIUM;
            else              size = AsteroidSize::LARGE;

            // 3) Separación mínima basada en radios (más correcta que MIN_SEP fijo)
            //    Ajusta el margen según quieras "campo" más o menos denso.
            float thisRadius = (size == AsteroidSize::LARGE)  ? 3.0f :
                               (size == AsteroidSize::MEDIUM) ? 2.0f : 1.2f;
            float margin = 1.0f; // baja a 0.3 si quieres mucha densidad

            bool tooClose = false;
            for (const auto& a : asteroids) {
                if (!a.alive) continue;
                Vec3 d = spawnPos - a.pos;
                float minDist = (thisRadius + a.radius + margin);
                if (d.dot(d) < minDist * minDist) { tooClose = true; break; }
            }
            if (tooClose) continue;

            // 4) Velocidad (tuya, pero puedes escalar con score si quieres)
            Vec3 vel = {
                ((rand() % 100) - 50) * 0.03f,
                ((rand() % 100) - 50) * 0.03f,
                -48.f - (rand() % 30)
            };

            asteroids.emplace_back(spawnPos, vel, size, renderMode, rand());
            ok = true;
        }

        // Fallback si el espacio está saturado
        if (!ok) {
            Vec3 fallbackPos = {R_MAX, 0.f, ship.pos.z + MIN_SPAWN_Z_AHEAD + SPAWN_Z_RANGE};
            Vec3 vel = {0.f, 0.f, -55.f};
            asteroids.emplace_back(fallbackPos, vel, AsteroidSize::SMALL, renderMode, rand());
        }
    }
}



void Game::render() {
    renderer.clear();

    camera.follow(ship.pos, ship.fwd, ship.up);
    Mat4 view = camera.viewMatrix();

    // Nave (parpadea mientras es invencible)
    bool drawShip = !ship.invincible || ((SDL_GetTicks() / 120) % 2 == 0);
    if (ship.alive && drawShip)
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
            // Modo HD: rasterización con textura o fallback a colores
            renderer.drawFilledMesh(a.getMesh(), a.worldTransform(), view, a.diffuseTexture);
        } else {
            // Modo LOW_POLY: wireframe con color por tamaño
            SDL_Color col = (a.size == AsteroidSize::LARGE)  ? SDL_Color{200, 120, 30, 255} :
                            (a.size == AsteroidSize::MEDIUM) ? SDL_Color{180, 160, 60, 255} :
                                                               SDL_Color{160, 200, 90, 255};
            renderer.drawMesh(a.getMesh(), a.worldTransform(), view, col);
        }
    }

    for (const auto& b : bullets)
        renderer.drawMesh(bulletMesh, b.worldTransform(), view, {255, 255, 50, 255});

    renderer.updateSpeedLines(m_dt, m_hyperIntensity);
    renderer.drawSpeedLines(m_hyperIntensity);

    renderer.present();
}

void Game::updateHUD() {
    const char* mode = (renderMode == RenderMode::HD) ? "HD" : "LOW_POLY";
    renderer.setWindowTitle(
        "Asteroids 3D  |  Score: " + std::to_string(score) +
        "  |  Lives: " + std::to_string(lives) +
        "  |  Mode: "  + std::string(mode) +
        "  |  [WASD/Arrows] move  [F] fire  [T] toggle HD"
    );
}

void Game::restart() {
    score     = 0;
    lives     = 3;
    wave      = 0;
    state     = GameState::PLAYING;
    spawnTimer = 0.5f;
    asteroids.clear();
    bullets.clear();
    ship.respawn();
    
    // Spawn inicial
    for (int i = 0; i < 15; i++) {
        spawnAsteroid();
    }
}