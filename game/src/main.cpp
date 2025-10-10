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
  testNetClient();
  return 0;
  RenderEngine renderer;

  int selection = start(&renderer);
  // Selection has been chosen
  switch (selection) {
    case 0: { startLocalGame(&renderer); break; }
    case 1: { std::cout << "Online not yet implemented" << std::endl; break; }
    case 2: { std::cout << "Settings not yet implemented" << std::endl; break; }
  }

  return 0;
}

int start(RenderEngine* renderer) {
  // Used to display FPS
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  double frame_rate = -1.0;
  Timer timer, fps_timer;
  timer.start(); fps_timer.start();
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
      frame_rate = (double) 1 / fps_timer.duration();
      renderer->FPS = frame_rate;
      fps_timer.start();
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


  return selection;
}

int startLocalGame(RenderEngine* renderer) {
  PlayerController* p1 = new Hunko();
  PlayerController* p2 = new Hunko();
  GameManager game(p1, p2, 1);

  // Testing local 2-player
  InputSystem* p1_inputs = new InputSystem();
  InputSystem* p2_inputs = new InputSystem();
  p2_inputs->setP2DefaultBindings();

  // Define duration of each frame
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  // Used to display FPS
  double frame_rate = -1.0;
  Timer game_timer, fps_timer;
  game_timer.start(); fps_timer.start();

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

      p1_inputs->updateButtonStates(&e);
      p2_inputs->updateButtonStates(&e);
    }

    // Cap frame rate at 60 fps
    if (game_timer.duration() >= (double) min_frame_duration*(game.cur_tick-INITIAL_FRAME)) {
    // if (game_timer.duration() >= 0) {
      // Reset Timer
      frame_rate = (double) 1 / fps_timer.duration();
      fps_timer.start();

      // Clean inputs based on previous input state
      handleButtonStateTick(game.allocator.getInputsAtTick(0, game.cur_tick-1), &p1_inputs->buttons);
      handleButtonStateTick(game.allocator.getInputsAtTick(1, game.cur_tick-1), &p2_inputs->buttons);

      // ROLLBACK FUNCTIONALITY DEMO
      // if (game.cur_tick > 120 && game.cur_tick % 20 == 0) {
      //   game.rollBack(game.cur_tick-20, &p2_inputs->buttons);
      // }

      // Send accumulated inputs to game engine
      game.updateInputs(&p1_inputs->buttons, 0);
      game.updateInputs(&p2_inputs->buttons, 1);

      // Move to next frame
      game.tick();

      // DEBUGGING
      PlayerEntity* p1 = game.getPlayer(0);
      PlayerEntity* p2 = game.getPlayer(1);

      // Verify Motion Interpreter
      // std::vector<NumPadDir> fqc_mot = {DOWN, DOWN_RIGHT, RIGHT};
      // ButtonStates* tmp_btns = &game.allocator.getCurrentScene()->inputs[0];
      // printMotionBuffer(tmp_btns->dir_buffer);
      // if (game.players[0]->checkMotionInputs(fqc_mot, 60, tmp_btns->dir_buffer)) {
      //   std::cout << "IT IS RISEN! HUZZAH! FQC IS ALIVE!\n";
      // } else {
      //   std::cout << "NOPE\n";
      // }

      // Verify inputs
      // std::cout << "Tick: " << game.allocator.cur_tick << " (" << game.cur_tick << ") ";
      // showButtonStates(&game.allocator.getCurrentScene()->inputs[0]);
      // printMotionBuffer(game.allocator.getCurrentScene()->inputs[0].dir_buffer);

      // Verify state
      std::cout << "P1 STATE: " << game.players[0]->getStateString(p1) 
                << " vmod: " << p1->v_mod << " xvel: " << p1->x_vel
                << std::endl; 
      // << " R=" << p1->f_recovery << " A=" << p1->f_active << " I=" << p1->f_invuln 
      // << " x_vel: " << p1->x_vel << std::endl;

      // std::cout << "P2 STATE: " << game.players[1]->getStateString(p2) 
      // << " R=" << p2->f_recovery << " A=" << p2->f_active << " I=" << p2->f_invuln 
      // << " x_vel: " << p2->x_vel << std::endl;

      // Only render if game engine is caught up
      if (game_timer.duration() <= (double) min_frame_duration*(game.cur_tick+1-INITIAL_FRAME)) {
        renderer->FPS = frame_rate;
        renderer->renderGameScene(&game);
      }
    }
  }
  return 1;
}
