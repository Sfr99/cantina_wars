#pragma once
#include <vector>
#include "Renderer.hpp"
#include "Camera.hpp"
#include "Ship.hpp"
#include "Asteroid.hpp"
#include "Bullet.hpp"
#include "music.hpp"

enum class GameState { PLAYING, GAME_OVER };
namespace audio { class MusicSystem; }

class Game {
public:
    explicit Game(audio::MusicSystem& audio);
    void run();


private:

    float m_dt = 0.f;
    float m_hyperIntensity = 0.f;
    struct HyperspaceState {
        float charge = 0.f;       // 0..1
        float chargeTime = 1.0f;  // seg hasta 1
        float decayTime  = 0.5f;  // seg hasta 0
        float maxMult    = 3.0f;  // multiplicador máximo
    };

    HyperspaceState hyperspace;

    audio::MusicSystem& m_audio;
    Renderer  renderer;
    Camera    camera;
    Ship      ship;

    std::vector<Asteroid> asteroids;
    std::vector<Bullet>   bullets;

    Mesh shipMesh;
    Mesh bulletMesh;

    int       score    = 0;
    int       lives    = 3;
    int       wave     = 0;
    bool      running  = true;
    GameState state    = GameState::PLAYING;
    RenderMode renderMode = RenderMode::LOW_POLY;
    float     spawnTimer  = 0.5f;

    void handleEvents();
    void update(float dt);
    void checkCollisions();
    void spawnAsteroid();
    void render();
    void updateHUD();
    void restart();
    void spawnWave();

    static bool sphereCollide(Vec3 a, float ra, Vec3 b, float rb);
};