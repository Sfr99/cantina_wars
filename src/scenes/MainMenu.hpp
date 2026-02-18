/*
 * scenes/MainMenu.hpp
 * Pantalla de inicio con fondo dinámico (parallax de estrellas + asteroides flotantes),
 * título "CANTINA WARS" y cinco botones. Solo JUGAR y SALIR son funcionales.
 * Navegación con ratón y teclado (flechas + Enter).
 */
#pragma once
namespace audio { class MusicSystem; }
#include "../core/GameConfig.hpp"
#include "DifficultyScreen.hpp"
#include <SDL2/SDL.h>
#include <vector>
#include "../rendering/Renderer.hpp"
#include "../rendering/Camera.hpp"
#include "../entities/Asteroid.hpp"
#include "../ui/Button.hpp"

enum class MenuResult { PLAY, QUIT };

class MainMenu {
public:
    MainMenu(audio::MusicSystem& audio, Renderer& renderer, GameConfig& config);
    ~MainMenu();

    /* Ejecuta el loop del menú y bloquea hasta que el usuario elige. */
    MenuResult run();

private:
    /* Capa de estrellas para el efecto parallax. */
    struct StarLayer {
        struct Star { float x, y; Uint8 brightness; };
        std::vector<Star> stars;
        float speed;   // px/s hacia abajo
        int   dotSize; // tamaño del punto en píxeles
    };

    audio::MusicSystem& m_audio;
    GameConfig&         m_config;
    Renderer&  renderer;
    Camera     camera;

    std::vector<Button>   m_buttons;
    std::vector<Asteroid> m_bgAsteroids;
    StarLayer             m_layers[3];

    int  m_selected = 0;   // índice del botón seleccionado por teclado
    bool m_done     = false;
    MenuResult m_result = MenuResult::QUIT;

    void buildButtons();
    void initBackground();

    void handleEvents();
    void update(float dt);
    void render();

    /* Actualiza posición de estrellas; las que salen por abajo reaparecen arriba. */
    void updateStars(float dt);

    /* Avanza los asteroides de fondo; los que se alejan demasiado reaparecen adelante. */
    void updateBgAsteroids(float dt);

    /* Dibuja las tres capas de estrellas con parallax. */
    void drawStarLayers();

    /* Dibuja los asteroides flotantes del fondo. */
    void drawBgAsteroids();

    /* Dibuja el título y todos los botones sobre el fondo. */
    /* Dibuja solo el fondo dinámico (estrellas + asteroides); usado también por DifficultyScreen. */
    void renderBackground();

    void drawUI();

    /* Dibuja un botón individual con su estado visual. */
    void drawButton(const Button& btn, bool selected);
};