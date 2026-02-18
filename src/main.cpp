#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "Game.hpp"
#include "audio/music.hpp"

int main(int argc, char* argv[]) {
    audio::MusicSystem audioSys;
    Game game(audioSys);
    game.run();
    return 0;
}