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
#include "core/GameConfig.hpp"
#include "core/RankingSystem.hpp"
#include "ui/HUD.hpp"
#include "ui/Overlay.hpp"

enum class GameState { PLAYING, GAME_OVER, NAME_ENTRY };
namespace audio { class MusicSystem; }

class Game {
public:
    Game(audio::MusicSystem& audio, Renderer& renderer, const GameConfig& config,
         RankingSystem& ranking);

    void run();

private:
    audio::MusicSystem& m_audio;
    Renderer&           renderer;
    GameConfig          m_config;
    RankingSystem&      m_ranking;
    Camera              camera;
    Ship                ship;

    std::vector<Asteroid> asteroids;
    std::vector<Bullet>   bullets;

    Mesh shipMesh;
    Mesh shipMeshHD;
    Mesh bulletMesh;

    HyperspaceSystem hyperspace;
    float            m_dt             = 0.f;
    float            m_hyperIntensity = 0.f;
    int              m_boostChannel   = -1;
    HUD              m_hud;
    Overlay          m_overlay;

    int        score       = 0;
    int        lives       = 3;
    int        wave        = 0;
    bool       running     = true;
    GameState  state       = GameState::PLAYING;
    RenderMode renderMode  = RenderMode::LOW_POLY;
    float      spawnTimer  = 0.5f;

    // New high score notification
    bool  m_newHighScore      = false;   // se activó durante esta partida
    float m_highScoreBannerT  = 0.f;    // temporizador del banner (segundos)
    int   m_prevTopScore      = 0;      // top score al iniciar, para detectar superación

    // Name entry state
    char m_nameEntry[3]   = {'A', 'A', 'A'};
    int  m_nameCursor     = 0;

    void handleEvents();
    void handleNameEntryEvent(const SDL_Event& e);
    void update(float dt);
    void checkCollisions();
    void spawnAsteroid();
    void render();
    void updateHUD();
    void restart();
    void spawnWave();
    void enterGameOver();
    void submitScore();

    void drawHighScoreBanner();

    static bool sphereCollide(Vec3 a, float ra, Vec3 b, float rb);
};