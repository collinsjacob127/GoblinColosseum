/**
 * Author: Jacob Collins
 * Description: Main file for Goblin Colosseum
 *  Handles initialization and cleanup of game.
 */
 
#include <iostream> // cout
// #include <cstdlib>  // read environment variables
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "engine/engine.hpp"
#include "net/net.hpp"
#include "render/render.hpp"

// #define CUSTOM_CHECK_RENDER_DRIVERS

// Skeleton of SDL basic calls provided by [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

int main(int argc, char* argv[]) {
    test_render_include_works();
    test_engine_include_works();
    test_net_include_works();

    // const char* cwd = std::getenv("GOBLIN_ROOT_CWD");
    // if (cwd) {
    //     std::cout << "Worked!" << std::endl;
    //     std::cout << "Goblin CWD: \n" << cwd << std::endl;
    // } else {
    //     std::cout << "Not worked!" << std::endl;
    // }

    /* INITIALIZATION OF RENDERER AND WINDOW */
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();

    SDL_Window* win = SDL_CreateWindow("Goblin Colosseum",1920, 1080, SDL_WINDOW_OPENGL);
    if (win == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        exit(EXIT_FAILURE);
    }

#ifdef CUSTOM_CHECK_RENDER_DRIVERS
    int n_drivers = SDL_GetNumRenderDrivers();
    for (int i = 0; i < n_drivers; i++) {
        std::cout << SDL_GetRenderDriver(i) << "\n";
    }
#endif

    SDL_Renderer* ren = SDL_CreateRenderer(win, "gpu");
    if (ren == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(win);
        SDL_Quit();
        exit(EXIT_FAILURE);
    }
    /* END INITIALIZATION OF RENDERER AND WINDOW */

    // Load a font
    TTF_Font *font = TTF_OpenFont("build/assets/fonts/OpenSans-Regular.ttf", 24);
    if (!font) {
        std::cerr << "Font load error: " << SDL_GetError() << std::endl;
        exit(EXIT_FAILURE);
    }

    SDL_Event e;
    bool quit = false;

    // Define a rectangle
    float x_pos = 150, y_pos = 100;

    SDL_FRect greenSquare {x_pos, y_pos, 100, 100};

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.key.key == SDLK_ESCAPE) {
                quit = true;
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); // Set render draw color to black
        SDL_RenderClear(ren); // Clear the renderer

        SDL_SetRenderDrawColor(ren, 0, 255, 0, 255); // Set render draw color to green
        SDL_RenderFillRect(ren, &greenSquare); // Render the rectangle
        
        SDL_RenderPresent(ren); // Render the screen
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
