/*
 * scenes/DifficultyScreen.hpp
 * Pantalla de selección de dificultad. Muestra tres opciones con descripción
 * y devuelve el control a MainMenu con la dificultad seleccionada escrita
 * en GameConfig. Reutiliza el mismo Renderer que el resto de escenas.
 */
#pragma once
#include <SDL2/SDL.h>
#include <functional>
#include "../core/GameConfig.hpp"
#include "../ui/Button.hpp"
#include "../rendering/Renderer.hpp"

class DifficultyScreen {
public:
    // bgCallback: función que dibuja el fondo (llamada cada frame antes del overlay)
    DifficultyScreen(Renderer& renderer, GameConfig& config,
                     std::function<void()> bgCallback = nullptr);

    /* Ejecuta el loop hasta que el usuario confirma o cancela. */
    void run();

private:
    Renderer&   m_renderer;
    GameConfig& m_config;

    std::vector<Button> m_buttons;
    int  m_selected = (int)Difficulty::NORMAL;
    bool m_done     = false;
    std::function<void()> m_bgCallback;

    void buildButtons();
    void handleEvents();
    void render();
    void drawOption(const Button& btn, Difficulty diff, bool selected) const;

    /* Descripción corta de cada nivel para mostrar bajo el botón. */
    static const char* description(Difficulty d);
};