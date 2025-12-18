/**
 * Author: Jacob Collins
 * Description: Main file for Goblin Colosseum

 */

#include <iostream>  // cout
#include <thread> // sleep(0) on windows
#include <csignal>

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
int onlineMenu(RenderEngine* renderer);
int startOnlineGame(RenderEngine* renderer);

// Global so cleanup can be guaranteed
NetEngine net_engine;
void handleUnexpectedClosure(int signal_num);

int main(int argc, char* argv[]) {
  std::signal(SIGINT, handleUnexpectedClosure);
  // std::signal(SIGABRT, handleUnexpectedClosure);
  // std::signal(SIGTERM, handleUnexpectedClosure);

  RenderEngine renderer;

  int selection = start(&renderer);
  // Selection has been chosen
  switch (selection) {
    case 0: { startLocalGame(&renderer); break; }
    case 1: { onlineMenu(&renderer); break; }
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

      // Send accumulated inputs to game engine
      game.updateInputs(&p1_inputs->buttons, 0);
      game.updateInputs(&p2_inputs->buttons, 1);

      // Move to next frame
      game.tick();

      // DEBUGGING
      // PlayerEntity* p1 = game.getPlayer(0);
      // PlayerEntity* p2 = game.getPlayer(1);

      // Verify Motion Interpreter
      // std::vector<NumPadDir> fqc_mot = {DOWN, DOWN_RIGHT, RIGHT};
      // ButtonStates* tmp_btns = &game.allocator.getCurrentScene()->inputs[0];
      // printMotionBuffer(tmp_btns->dir_buffer);
      // if (game.players[0]->checkMotionInputs(fqc_mot, 60, tmp_btns->dir_buffer)) {
      //   std::cout << "IT IS RISEN! HUZZAH! FQC IS ALIVE!\n";
      // } else {
      //   std::cout << "NOPE\n";
      // }
      // printMotionBuffer(game.allocator.getCurrentScene()->inputs->dir_buffer);
      // Verify inputs
      // std::cout << "Tick: " << game.allocator.cur_tick << " (" << game.cur_tick << ") ";
      // showButtonStates(&game.allocator.getCurrentScene()->inputs[0]);
      
      // printMotionBuffer(game.allocator.getCurrentScene()->inputs[0].dir_buffer);

      // Verify state
      // std::cout << "P1 STATE: " << game.players[0]->getStateString(p1) 
      //           // << " vmod: " << p1->v_mod << " xvel: " << p1->x_vel
      //           << " block: " << p1->block 
      //           << " health: " << p1->health 
      //           << std::endl; 

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

int onlineMenu(RenderEngine* renderer) {
  // TODO: 
  // Select p1 vs p2
  // Select character
  // Send / recv inputs
  // Display lobby list
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  double frame_rate = -1.0;
  Timer timer, fps_timer;
  timer.start(); fps_timer.start();
  unsigned long long n_ticks = 1;
  
  /*
  START NET TEST 
  */
   
  renderer->renderOnlineMenu(&net_engine);
  net_engine.getLocalUserName();
  net_engine.getPlayerSelection();
  // Handle server connection & p2p initialization
  net_engine.testNetClient(); // Waits and clears buffer afterwards

  COLORS.printSuccess("\nGame Starting...\n");

  // return 0;

  return startOnlineGame(renderer);
}

struct RemoteInputNode {
  bool been_received = false;
  bool been_applied = false;
  ButtonStates btns;
};

// References rollback pseudocode by rcmagic: https://gist.github.com/rcmagic/f8d76bca32b5609e85ab156db38387e9
struct RollbackTracker {
  ssize_t local_frame = INITIAL_FRAME;    // Latest updated frame
  ssize_t remote_frame = INITIAL_FRAME;   // Latest frame received from remote
  ssize_t sync_frame = INITIAL_FRAME;     // Last frame where sync occured (known that all inputs recvd through here)
  ssize_t rb_frame = INITIAL_FRAME;       // Rollbacks have been applied up to at least this point
  ssize_t remote_frame_advantage = 0;     // Latest frame adv. received

  bool rollbackCondition() {
    // No ned to rollback is we don't have frame after the previous sync
    return (local_frame > sync_frame) && (remote_frame > sync_frame);
  }

  bool timeSynced() {
    // How far the client is ahead of last recvd frame
    ssize_t local_frame_advantage = local_frame - remote_frame;

    // In the guide it says this is reported by peer, but i'll just hazard estimate it
    ssize_t frame_advantage_difference = local_frame_advantage - remote_frame_advantage;

    return (local_frame_advantage < MAX_ROLLBACK_FRAMES) && (frame_advantage_difference <= FRAME_ADVANTAGE_LIMIT);
  }

  void updateSyncFrame(std::vector<RemoteInputNode> &remote_input_list) {
    ssize_t final_frame = remote_frame;
    if (remote_frame > local_frame) {
      final_frame = local_frame;
    }
    // This should instead check the game allocator and only rollback when a prediction failed
    // Currently marks based on recvd y/n instead of correct inputs y/n
    ssize_t tmp_sync_frame = final_frame;
    for (ssize_t found_frame = sync_frame+1; found_frame <= final_frame; ++found_frame) {
      if (!remote_input_list[found_frame].been_received) {
        tmp_sync_frame = found_frame-1;
        break;
      }
    }
    sync_frame = tmp_sync_frame;
  }

  void executeRollbacks(GameManager* game, std::vector<RemoteInputNode> &remote_input_list) {
    for (ssize_t f = rb_frame+1; f <= sync_frame; ++f) {
      game->rollBack(f, &remote_input_list[f].btns);
      remote_input_list[f].been_applied = true;
    }
  }
};

int startOnlineGame(RenderEngine* renderer) {
  // Tracker for rollback
  RollbackTracker rb_tracker;

  // Structure for storing remote inputs
  std::vector<RemoteInputNode> remote_inputs_list(MAX_GAME_DURATION);

  // Character selection
  PlayerController* p1 = new Hunko();
  PlayerController* p2 = new Hunko();

  // Game initialization
  GameManager game(p1, p2, (net_engine.p_num == 1 ? 2 : 1));
  
  // Input startup
  InputSystem* local_inputs = new InputSystem();

  // Define duration of each frame
  double min_frame_duration = (double)1 / (double)FRAME_RATE_CAP;
  // Used to display FPS
  double frame_rate = -1.0;
  Timer game_timer, fps_timer;
  game_timer.start(); fps_timer.start();

  // MAIN GAME LOOP
  bool quit = false;
  while (!quit) {

    // Handle Local Events / Hardware Inputs
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      // Let the game quit lol
      if (e.type == SDL_EVENT_QUIT) { quit = true; }
      // Escape will quit also
      if (e.type == SDL_EVENT_KEY_DOWN)
        if (e.key.key == SDLK_ESCAPE) { quit = true; }
      if (e.type == SDL_EVENT_WINDOW_RESIZED) { renderer->calculateScale(e.window.data1, e.window.data2); }
      // Send keyboard to game inputs

      local_inputs->updateButtonStates(&e);
    }

    // Update Network
    std::pair<bool, NetInputs> net_response = net_engine.recvPeerInputs();
    if (!net_response.first) {
      //// Received no (or bad) packet ////

    } else if (net_response.second.is_repeat_request) {
      //// Received repeat request ////
      // What frame we need?
      uint16_t f_requested = net_response.second.parse().second;
      // This line might not be right - testing imputation of remote frame advantage
      rb_tracker.remote_frame_advantage = rb_tracker.remote_frame - (ssize_t)f_requested;
      if (ENABLE_NET_INPUT_HANDLER_DEBUGS)
        printf("[DEBUG] Opponent requested repeat send of inputs from frame %u\n", f_requested);

      ssize_t bytes_sent = 0;
      // Only respond to requests that are for frames we have
      if (f_requested <= game.cur_tick) {
        // Get local inputs from that frame
        const ButtonStates *req_inputs = game.allocator.getInputsAtTick(game.loc_pindex, f_requested);
        // Build packet with the inputs
        NetInputs out_pkt(false, f_requested, req_inputs);
        // Send it
        bytes_sent = net_engine.sendNetInputs(out_pkt);
      }

      // Check that it sent right
      if (ENABLE_NET_INPUT_HANDLER_DEBUGS && bytes_sent <= 0) { 
        COLORS.printError("[Error] Failed to send requested net inputs\n"); 
      }

    } else {
      //// Received valid input from peer ////
      std::pair<ButtonStates, uint16_t> remote_pkt = net_response.second.parse();
      // Don't overwrite valid inputs (anything we've already received)
      if (!remote_inputs_list[remote_pkt.second].been_received) {
        // Save the inputs we got
        remote_inputs_list[remote_pkt.second].btns = remote_pkt.first;
        remote_inputs_list[remote_pkt.second].been_received = true;
        // Set remote frame to the highest frame received by peer
        rb_tracker.remote_frame = std::max((ssize_t)remote_pkt.second, rb_tracker.remote_frame);
        // This *should* be sent from the peer, but let's try and see if we can do without
        // Instead, updating remote_frame_advantage when we receive a repeat request
        // rb_tracker.remote_frame_advantage = rb_tracker.local_frame - rb_tracker.remote_frame;
      }
    }

    // Update synchronization
    rb_tracker.updateSyncFrame(remote_inputs_list);

    // Rollback if necessary
    if (rb_tracker.rollbackCondition()) {
      rb_tracker.executeRollbacks(&game, remote_inputs_list);
    }

    // Verify p2p synchronization
    if (!rb_tracker.timeSynced()) {
      if (ENABLE_NET_INPUT_HANDLER_DEBUGS) {
        printf("GAME STATES NOT SYNCHRONIZED - WAITING TO SYNC...\n");
        std::cout << "[RB Tracker] local_frame=: " << rb_tracker.local_frame << std::endl;
        std::cout << "[RB Tracker] remote_frame: " << rb_tracker.remote_frame << std::endl;
        std::cout << "[RB Tracker] sync_frame==: " << rb_tracker.sync_frame << std::endl;
        std::cout << "[RB Tracker] rb_frame====: " << rb_tracker.rb_frame << std::endl;
        std::cout << std::endl;
      }
      crossPlatformSleep(5);
      continue;
    }

    // Game tick:
    if (game_timer.duration() >= (double) min_frame_duration*(game.cur_tick-INITIAL_FRAME)) {
      // Reset Timer
      frame_rate = (double) 1 / fps_timer.duration();
      fps_timer.start();

      // Send accumulated inputs to game engine
      game.updateInputs(&local_inputs->buttons, game.loc_pindex);

      // Move to next frame
      game.tick();

      NetInputs cur_inputs(false, game.cur_tick, &local_inputs->buttons);
      net_engine.sendNetInputs(cur_inputs);

      // Only render if game engine is caught up
      if (game_timer.duration() <= (double) min_frame_duration*(game.cur_tick+1-INITIAL_FRAME)) {
        renderer->FPS = frame_rate;
        renderer->renderGameScene(&game);
      }
    }

  } // Game loop

  return 1;
}

void handleUnexpectedClosure(int signal_num) {
  if (!continue_program) { exit(0); }
  continue_program = false;
  std::cout << std::flush << COLORS.red_fg;
  std::cout << "\nRecieved SIGINT.\n";
  // net_engine.~NetEngine();
  // renderer.~RenderEngine();
  std::cout << std::flush << COLORS.green_fg;
  std::cout << "Exiting safely...\n";
  std::cout << COLORS.white_fg;
  exit(0);
}
