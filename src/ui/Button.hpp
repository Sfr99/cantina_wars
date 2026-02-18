/*
 * ui/Button.hpp
 * Datos y estado de un botón de menú. No contiene lógica de dibujado;
 * el renderizado se hace en la escena que lo usa.
 */
#pragma once
#include <SDL2/SDL.h>
#include <string>

enum class ButtonState { IDLE, HOVER, DISABLED };

struct Button {
    std::string  label;
    SDL_Rect     rect;      // posición y tamaño en pantalla
    ButtonState  state    = ButtonState::IDLE;
    bool         enabled  = true;

    /* Devuelve true si el punto (px, py) está dentro del botón. */
    bool contains(int px, int py) const {
        return px >= rect.x && px < rect.x + rect.w
            && py >= rect.y && py < rect.y + rect.h;
    }
};