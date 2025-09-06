/**
 * Author: Jacob Collins
 * Description: Main file for Goblin Colosseum

 */

#include <iostream>  // cout
// #include <cstdlib>  // read environment variables
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "engine/engine.hpp"
#include "net/net.hpp"
#include "render/render.hpp"

// #define CUSTOM_CHECK_RENDER_DRIVERS

// Skeleton of SDL basic calls provided by
// [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

int main(int argc, char* argv[]) {
  /* INITIALIZATION OF RENDERER AND WINDOW */
  RenderEngine renderer;
  /* END INITIALIZATION OF RENDERER AND WINDOW */
  PlayerState player;
  PlayerState dummy;

  InputSystem inputSystem;

  // MAIN GAME LOOP
  bool quit = false;
  while (!quit) {
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
      inputSystem.updateInputState(&e);
    }
    // Pass inputs to player
    player.processInputs(&inputSystem.inputState);
    player.computeNextState();

    // Render player
    renderer.renderPlayer(&player);

    SDL_RenderPresent(renderer.ren);  // Render the screen

    // inputSystem.inputState.reset();
    // Only attack on-press
    inputSystem.inputState.attack = false;
  }

  SDL_DestroyRenderer(renderer.ren);
  SDL_DestroyWindow(renderer.win);
  SDL_Quit();

  return 0;
}
