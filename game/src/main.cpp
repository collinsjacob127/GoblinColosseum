/**
 * Author: Jacob Collins
 * Description: Main file for Goblin Colosseum
 *  Handles initialization and cleanup of game.
 */
 
#include <iostream>
#include <SDL3/SDL.h>

#include "render/render.hpp"
#include "net/net.hpp"
#include "engine/engine.hpp"

// #define CUSTOM_CHECK_RENDER_DRIVERS

// Skeleton of SDL initialization provided by [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

int main(int argc, char* argv[]) {
    test_render_include_works();
    test_engine_include_works();
    test_net_include_works();

    /* INITIALIZATION OF RENDERER AND WINDOW */
    SDL_Init(SDL_INIT_VIDEO);

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

    SDL_Event e;
    bool quit = false;

    // Define a rectangle
    SDL_FRect greenSquare {270, 190, 100, 100};

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
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
