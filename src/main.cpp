#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "rendering/Renderer.hpp"
#include "Game.hpp"
#include "scenes/MainMenu.hpp"
#include "audio/music.hpp"
#include "core/GameConfig.hpp"
#include "core/RankingSystem.hpp"

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    Renderer renderer(900, 600);
    if (!renderer.init()) { IMG_Quit(); SDL_Quit(); return 1; }

    audio::MusicSystem audioSys;
    audioSys.init();

    GameConfig    config;
    RankingSystem ranking;  // carga scores.txt automáticamente

    bool quit = false;
    while (!quit) {
        {
            MainMenu menu(audioSys, renderer, config, ranking);
            MenuResult result = menu.run();
            if (result == MenuResult::QUIT) { quit = true; break; }
        }
        {
            Game game(audioSys, renderer, config, ranking);
            game.run();
        }
    }

    IMG_Quit();
    SDL_Quit();
    return 0;
}