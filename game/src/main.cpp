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
  RenderEngine renderer;
  GameManager game_manager;

  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  double frame_rate = -1.0;
  Timer game_timer;
  game_timer.start();

  // MAIN GAME LOOP
  bool quit = false;
  while (!quit) {

    // Handle Events / Hardware Inputs
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      // Let the game quit lol
      if (e.type == SDL_EVENT_QUIT) { quit = true; }
      // Escape will quit also
      if (e.type == SDL_EVENT_KEY_DOWN)
        if (e.key.key == SDLK_ESCAPE) { quit = true; }
      // Send keyboard to game inputs
      game_manager.updateLocalInputs(&e);
    }

    // Cap frame rate at 60 fps
    if (game_timer.duration() >= min_frame_duration) {
    // if (game_timer.duration() >= 0) {
      // Reset Timer
      frame_rate = (double) 1 / game_timer.duration();
      game_timer.start();

      // Move to next frame
      game_manager.tick();

      // Clear screen
      renderer.clearScreen();

      // Render player
      renderer.renderPlayer(game_manager.getP1());
      renderer.renderPlayer(game_manager.getP2());
      std::cout << game_manager.game_allocator.cur_tick << std::endl;
      renderer.displayFPS(frame_rate);
      SDL_RenderPresent(renderer.ren);  // Render the screen
    }
  }

  SDL_DestroyRenderer(renderer.ren);
  SDL_DestroyWindow(renderer.win);
  SDL_Quit();

  return 0;
}
