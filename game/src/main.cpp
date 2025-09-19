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
#include "characters/characters.hpp"

#define FRAME_RATE_CAP 60

// Skeleton of SDL basic calls provided by
// [glusoft](https://glusoft.com/sdl3-tutorials/install-sdl3-linux-cmake/)

/**
 *  Display the start screen
 * Contains menu options:
 * - Local
 * - Online
 * - Settings
 * - Quit
 */ 
int start(RenderEngine* renderer);
int startLocalGame(RenderEngine* renderer);

int main(int argc, char* argv[]) {
  RenderEngine renderer;

  start(&renderer);
  // startLocalGame(&renderer);

  return 0;
}

int start(RenderEngine* renderer) {
  // Used to display FPS
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  double frame_rate = -1.0;
  Timer timer, fps_timer;
  timer.start();
  unsigned long long n_ticks = 1;

  InputSystem inputs;
  /**
   * nothing: -1
   * local: 0
   * online: 1
   * settings: 2
   * quit: 3
   */
  int selection = 0;
  int n_selections = 4;
  bool clicked = false;
  Coordinate mouse_pos;

  while (!clicked) {
    // Handle mouse events
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_EVENT_QUIT: { selection = 3; clicked = true; break; }
        case SDL_EVENT_KEY_DOWN: { if (e.key.key == SDLK_ESCAPE) { selection = 3; clicked = true;} break; }
        case SDL_EVENT_WINDOW_RESIZED: { renderer->calculateScale(e.window.data1, e.window.data2); break;}
      }
      // Send keyboard to game inputs
      inputs.updateButtonStates(&e);
    }

    // Once per frame, display everything
    if (timer.duration() >= (double) min_frame_duration*n_ticks) {
      n_ticks++;
      // Navigate with keyboard
      if (inputs.buttons.up) {
        selection = (n_selections + selection - 1) % n_selections;
      } else if (inputs.buttons.down) {
        selection = (selection + 1) % n_selections;
      }
      if (inputs.buttons.b3) { clicked = true; }
      inputs.resetButtonStates();
      renderer->renderStartMenu(selection);
    }
  }

  // Selection has been chosen
  switch (selection) {
    case 0: { startLocalGame(renderer); break; }
    case 1: { std::cout << "Online not yet implemented" << std::endl; break; }
    case 2: { std::cout << "Settings not yet implemented" << std::endl; break; }
  }

  return 0;
}

int startLocalGame(RenderEngine* renderer) {
  GameManager game;
  game.players[0] = new Hunko();
  game.players[1] = new Hunko();

  // Testing local 2-player
  InputSystem p2_inputs;
  p2_inputs.setP2DefaultBindings();

  // Define duration of each frame
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  // Used to display FPS
  double frame_rate = -1.0;
  Timer game_timer, fps_timer;
  game_timer.start();
  fps_timer.start();

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
      if (e.type == SDL_EVENT_WINDOW_RESIZED) { renderer->calculateScale(e.window.data1, e.window.data2); }
      // Send keyboard to game inputs
      game.updateLocalInputs(&e);

      p2_inputs.updateButtonStates(&e);
      game.rollBack(game.cur_tick, &p2_inputs.buttons);
    }

    // Cap frame rate at 60 fps
    if (game_timer.duration() >= (double) min_frame_duration*game.cur_tick) {
    // if (game_timer.duration() >= 0) {
      // Reset Timer
      frame_rate = (double) 1 / fps_timer.duration();
      fps_timer.start();

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

      // Debug inputs
      // std::cout << "Tick: " << game.allocator.cur_tick << " (" << game.cur_tick << ")\n";
      // showButtonStates(&(game.inputs[0].buttons));
      // std::cout << std::endl;

      // Move to next frame
      game.tick();
      // Only render if game is caught up to expected time of current frame
      if (game_timer.duration() <= (double) min_frame_duration*(game.cur_tick+1)) {
        renderer->renderGameScene(&game);
      }
    }
  }
  return 1;
}
