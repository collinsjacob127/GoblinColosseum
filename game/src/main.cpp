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

// Skeleton of SDL basic calls provided by
// [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

int startLocalGame(RenderEngine* renderer);

int main(int argc, char* argv[]) {
  RenderEngine renderer;

  startLocalGame(&renderer);

  SDL_DestroyRenderer(renderer.ren);
  SDL_DestroyWindow(renderer.win);
  SDL_Quit();

  return 0;
}

int startLocalGame(RenderEngine* renderer) {
  GameManager game;
  // Set player colors
  game.getPlayer(0)->disp_r = 3.0;
  game.getPlayer(0)->disp_g = 223.0;
  game.getPlayer(0)->disp_b = 252.0;

  game.getPlayer(1)->disp_r = 250.0;
  game.getPlayer(1)->disp_g = 161.0;
  game.getPlayer(1)->disp_b = 3.0;

  // TODO: Delete this, it's for testing rollback
  ButtonStates p2_dummy_buttons;
  p2_dummy_buttons.up = true;

  // Define duration of each frame
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  // Used to display FPS
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
      game.updateLocalInputs(&e);
    }

    // Cap frame rate at 60 fps
    if (game_timer.duration() >= min_frame_duration) {
    // if (game_timer.duration() >= 0) {
      // Reset Timer
      frame_rate = (double) 1 / game_timer.duration();
      game_timer.start();

      // ROLLBACK FUNCTIONALITY DEMO
      // if (game.cur_tick > 350 && game.cur_tick % 20 == 0) {
      //   std::cout << "pre-rollback" << std::endl;
      //   std::cout << "  game cur tick: " << game.cur_tick << "\n"
      //             << "  aloc cur tick: " << game.allocator.cur_tick << "\n";
      //   game.rollBack(game.cur_tick - 300, &p2_dummy_buttons);
      //   std::cout << "post-rollback" << std::endl;
      //   std::cout << "  game cur tick: " << game.cur_tick << "\n"
      //             << "  aloc cur tick: " << game.allocator.cur_tick << "\n";
      // }

      // Move to next frame
      game.tick();

      // Clear screen
      renderer->clearScreen();

      // Render player
      renderer->renderPlayer(game.getPlayer(0)); // Player 1
      renderer->renderPlayer(game.getPlayer(1)); // Player 2
      std::cout << game.allocator.cur_tick << " " << game.cur_tick << std::endl;
      renderer->displayFPS(frame_rate);
      SDL_RenderPresent(renderer->ren);  // Render the screen
    }
  }
  return 1;
}