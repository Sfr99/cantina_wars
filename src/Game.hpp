/*
 * Game.hpp
 * Coordinador principal del juego. Gestiona el loop, eventos, actualización,
 * colisiones y renderizado. El spawning se delega a Spawner y el boost a HyperspaceSystem.
 */
#pragma once
#include <vector>
#include "rendering/Renderer.hpp"
#include "rendering/Camera.hpp"
#include "rendering/GLTFLoader.hpp"
#include "entities/Ship.hpp"
#include "entities/Asteroid.hpp"
#include "entities/Bullet.hpp"
#include "core/HyperspaceSystem.hpp"
#include "ui/HUD.hpp"
#include "ui/Overlay.hpp"

enum class GameState { PLAYING, GAME_OVER };
namespace audio { class MusicSystem; }

class Game {
public:
    Game(audio::MusicSystem& audio, Renderer& renderer);

    /* Inicializa renderer y audio, lanza el loop principal. Bloquea hasta que el juego cierra. */
    void run();

private:
    audio::MusicSystem& m_audio;
    Renderer&           renderer;
    Camera              camera;
    Ship                ship;

    std::vector<Asteroid> asteroids;
    std::vector<Bullet>   bullets;

    Mesh shipMesh;
    Mesh shipMeshHD;
    Mesh bulletMesh;

    HyperspaceSystem hyperspace;
    float            m_dt            = 0.f;
    float            m_hyperIntensity = 0.f;
    int              m_boostChannel  = -1;
    HUD     m_hud;
    Overlay m_overlay;

    int        score       = 0;
    int        lives       = 3;
    int        wave        = 0;
    bool       running     = true;
    GameState  state       = GameState::PLAYING;
    RenderMode renderMode  = RenderMode::LOW_POLY;
    float      spawnTimer  = 0.5f;

    void handleEvents();
    void update(float dt);
    void checkCollisions();
    void spawnAsteroid();
    void render();
    void updateHUD();
    void restart();
    void spawnWave();

    /* Colisión esfera-esfera: devuelve true si las esferas se solapan. */
    static bool sphereCollide(Vec3 a, float ra, Vec3 b, float rb);
};