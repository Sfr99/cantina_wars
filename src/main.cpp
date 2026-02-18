#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "rendering/Renderer.hpp"
#include "Game.hpp"
#include "scenes/MainMenu.hpp"
#include "audio/music.hpp"

int main(int argc, char* argv[]) {
    // SDL y sus subsistemas se inician aquí y mueren al final; ninguna escena
    // los toca, así se evita el parpadeo de ventana y la pérdida de audio.
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    Renderer renderer(900, 600);
    if (!renderer.init()) {
        IMG_Quit();
        SDL_Quit();
        return 1;
    }

    audio::MusicSystem audioSys;
    audioSys.init();

    bool quit = false;
    while (!quit) {
        {
            MainMenu menu(audioSys, renderer);
            MenuResult result = menu.run();
            if (result == MenuResult::QUIT) { quit = true; break; }
        }
        {
            Game game(audioSys, renderer);
            game.run();
            // Al volver del juego se muestra el menú de nuevo
        }
    }

    IMG_Quit();
    SDL_Quit();
    return 0;
}