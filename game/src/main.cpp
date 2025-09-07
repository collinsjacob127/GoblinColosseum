/**
 * Author: Jacob Collins
 * Description: Main file for Goblin Colosseum

 */

#include <iostream>  // cout
#include <thread> // sleep(0) on windows

// #include <cstdlib>  // read environment variables
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "util/util.hpp"
#include "engine/engine.hpp"
#include "net/net.hpp"
#include "render/render.hpp"

#define FRAME_RATE_CAP 60

void highPrecisionSleep(double duration);

// Skeleton of SDL basic calls provided by
// [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

int main(int argc, char* argv[]) {
  /* INITIALIZATION OF RENDERER AND WINDOW */
  RenderEngine renderer;
  /* END INITIALIZATION OF RENDERER AND WINDOW */
  PlayerState player;
  PlayerState dummy;

  InputSystem input_system;

  Timer game_timer;

  // MAIN GAME LOOP
  bool quit = false;
  while (!quit) {
    game_timer.start();

    // Clear screen each new frame
    renderer.clearScreen();

    // HANDLE EVENTS
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      // Let the game quit lol
      if (e.type == SDL_EVENT_QUIT) { quit = true; }
      // Escape will quit also
      if (e.type == SDL_EVENT_KEY_DOWN)
        if (e.key.key == SDLK_ESCAPE) { quit = true; }
      // Send keyboard to game inputs
      input_system.updateInputState(&e);
    }
    // Pass inputs to player
    player.processInputs(&input_system.inputState);
    
    // Handle Events
    player.computeNextState();

    // Render player
    renderer.renderPlayer(&player);

    SDL_RenderPresent(renderer.ren);  // Render the screen

    // input_system.inputState.reset();
    // Only attack on-press
    input_system.inputState.attack = false;

    // Cap Render Frame Rate
    while (((double)1 / (double)FRAME_RATE_CAP) - game_timer.duration() > 0.001) {
      #ifdef _WIN32
        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
      #endif
    }
  }

  SDL_DestroyRenderer(renderer.ren);
  SDL_DestroyWindow(renderer.win);
  SDL_Quit();

  return 0;
}
