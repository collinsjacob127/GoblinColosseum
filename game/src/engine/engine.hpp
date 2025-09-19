/**
 * Author: Jacob Collins
 * Description:
 * This file contains headers for:
 * 
 * - Input System
 * The input system is an abstraction layer between system input and input commands
 * Based on input system designed by rcmagic (wrote GGST netcode)
 * https://github.com/rcmagic/DemoFighterWithNetcode/blob/master/game/InputSystem.lua
 * 
 * - Game Allocator
 * Allocates game state memory buffer, enabling rollback
 * Essentially an interface for the list of game scenes
 * 
 * - GameScene
 * Tracks entities and all information that is used for a given scene
 * 
 * - PlayerEntity
 * Struct containing information regarding the player's dynamic state
 * 
 * - Game Manager
 * Interface between main() game loop and the game engine
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <string.h>
#include <SDL3/SDL.h>
#include "util.hpp"

#define ENABLE_HELPER_PRINTOUTS true

// For rollback functionality demo:
#define MAX_ROLLBACK_FRAMES 60
// #define MAX_ROLLBACK_FRAMES 60
#define FRAME_ADVANTAGE_LIMIT 5
#define INITIAL_FRAME 0

#define MAX_INPUT_FRAMES 60

#define DEFAULT_XDIM 3200
#define DEFAULT_YDIM 1800

#define GAME_BORDER_X0 180
#define GAME_BORDER_X1 3020
#define GAME_BORDER_Y0 0
#define GAME_BORDER_Y1 1678

struct Keybinds {
  SDL_Scancode up = SDL_SCANCODE_W;
  SDL_Scancode down = SDL_SCANCODE_S;
  SDL_Scancode left = SDL_SCANCODE_A;
  SDL_Scancode right = SDL_SCANCODE_D;
  SDL_Scancode b1 = SDL_SCANCODE_U; // ps square
  SDL_Scancode b2 = SDL_SCANCODE_I; // ps triangle
  SDL_Scancode b3 = SDL_SCANCODE_J; // ps X
  SDL_Scancode b4 = SDL_SCANCODE_K; // ps circle
  SDL_Scancode l1 = SDL_SCANCODE_O; // left bumper
  SDL_Scancode r1 = SDL_SCANCODE_L; // right bumper
  // SDL_Scancode l2 = SDL_SCANCODE_P; // left trigger
  SDL_Scancode l2 = SDL_SCANCODE_LSHIFT; // left trigger
  SDL_Scancode r2 = SDL_SCANCODE_SEMICOLON; // right trigger
};

struct ButtonStates {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  // PS square
  bool b1 = false; 
  // ps triangle
  bool b2 = false; 
  // ps X
  bool b3 = false; 
  // ps circle
  bool b4 = false; 
  bool l1 = false;
  bool r1 = false;
  bool l2 = false;
  bool r2 = false;
};

// Used for debugging
void showButtonStates(const ButtonStates* btn);

class InputSystem {
 public:
  ButtonStates buttons;
  Keybinds bindings;

  InputSystem();
  void updateButtonStates(const SDL_Event *e);
  void resetButtonStates();
};

/**
 * Struct for dynamic character info
 */
struct PlayerEntity {
  std::string state = "STANDING";
  float x_pos = 1920.0/2.0;
  float y_pos = 1080.0/2.0;

  bool facing_right = true;

  float x_vel = 0.0;
  float y_vel = 0.0;

  float width = 100.0;
  float height = 300.0;
  int air_action_cnt = 0;
  int air_action_max = 2;

  int f_startup = 0;
  int f_active = 0;
  int f_recovery = 0;
};

/**
 * Base class for static character info
 */
class PlayerController {
 public:
  PlayerController();
  
  float walking_v = 10.0;
  float running_v = 25.0;
  float fastfall_v = 3.0;

  float airdash_v = 20.0;
  int f_airdash_recovery = 5;

  float jumping_v = -35.0;
  int f_jumping_recovery = 5;

  float gravity = 2.5;
  float friction = 2.5;

  float backdash_v = -25.0;
  int f_backdash_recovery = 20.0;

  virtual void testCharacterInclude();
  void setActions(PlayerEntity* p, const ButtonStates* in);
  bool isActionable(PlayerEntity* p);
  bool isGrounded(PlayerEntity* p);
  void updateFrames(PlayerEntity* p, const ButtonStates* in);
  bool holdingForward(const PlayerEntity* p, const ButtonStates* in);
  bool holdingBack(const PlayerEntity* p, const ButtonStates* in);
  void applyMovement(PlayerEntity* p);
 private:
  
};

struct GameScene {
  PlayerEntity players[2];  
  ButtonStates inputs[2];
};

class GameAllocator {
 public: 
  unsigned int cur_tick;
  // If online player is p1 => 0
  // If online player is p2 => 1
  // If no online player => 99
  unsigned int loc_pindex;
  unsigned int net_pindex;
  GameAllocator();
  GameAllocator(unsigned int net_p1_or_p2);

  GameScene* getCurrentScene();

  /**
   * @brief Function to move on to the next scene.
   * Copies current scene to next index, increments cur_tick,
   * and returns the *new* current scene.
   */
  GameScene* getNextScene();

  /**
   * @brief Function to roll back to a given frame.
   * @param prev_tick The frame to roll back to (sets allocator's cur_tick to this)
   * @param in The inputs to insert.
   * @note After calling rollback, engine must simulate back to current frame. 
   */
  GameScene* rollBack(unsigned int prev_tick, const ButtonStates* in);

  /**
   * @brief Roll forward to the next frame, duplicating net player's input,
   * and without changing local player's previous inputs.
   */
  GameScene* rollForward();

 private:
  GameScene history_buffer[MAX_ROLLBACK_FRAMES];
  unsigned int getCurrentIndex();
};

class GameManager {
 public:
  GameAllocator allocator;
  InputSystem inputs[2];
  PlayerController* players[2];

  unsigned int loc_pindex;
  unsigned int net_pindex;
  unsigned int cur_tick;

  GameManager();
  GameManager(unsigned int net_p1_or_p2);

  void updateLocalInputs(SDL_Event* e);
  void tick();
  void rollBack(unsigned int frame, const ButtonStates* in);

  PlayerEntity* getPlayer(unsigned int pid);

 private:
  void applyTickUpdates(GameScene* scene);
  void setFacingDir(PlayerEntity* p1, PlayerEntity* p2);
};
