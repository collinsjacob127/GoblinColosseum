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

#define ENABLE_HELPER_PRINTOUTS true

#define MAX_ROLLBACK_FRAMES 60
#define FRAME_ADVANTAGE_LIMIT 5
#define INITIAL_FRAME 0

#define MAX_INPUT_FRAMES 60

#define ENV_DIM_WALL_THICKNESS 20
#define ENV_DIM_FLOOR_HEIGHT 880
#define ENV_DIM_LEFT_WALL_X 0
#define ENV_DIM_RIGHT_WALL_X 1900

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
  SDL_Scancode l2 = SDL_SCANCODE_P; // left trigger
  SDL_Scancode r2 = SDL_SCANCODE_SEMICOLON; // right trigger
};

struct ButtonStates {
  bool up = false;
  bool down = false;
  bool left = false;
  bool right = false;
  bool b1 = false; // ps square
  bool b2 = false; // ps triangle
  bool b3 = false; // ps X
  bool b4 = false; // ps circle
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
};

struct PlayerEntity {
  float x_pos = 1920.0/2.0;
  float y_pos = 1080.0/2.0;

  float x_vel = 0.0;
  float y_vel = 0.0;

  float width = 100.0;
  float height = 300.0;

  float walking_v = 15.0;
  float jumping_v = -35.0;
  float fastfall_v = 3.0;

  int f_startup = 0;
  int f_active = 0;
  int f_recovery = 0;

  float gravity = 2.5;
  float friction = 5.0;

  float disp_r = 0.0;
  float disp_g = 200.0;
  float disp_b = 20.0;
};

// TODO: Implement rollback :)
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

  unsigned int loc_pindex;
  unsigned int net_pindex;
  unsigned int cur_tick;

  GameManager();
  GameManager(unsigned int net_p1_or_p2);

  void updateLocalInputs(SDL_Event* e);
  void setActions(PlayerEntity* p, const ButtonStates* in);
  void tick();
  void rollBack(unsigned int frame, const ButtonStates* in);

  PlayerEntity* getPlayer(unsigned int pid);

 private:
  void applyMovement(PlayerEntity* p);
  void updateFrames(PlayerEntity* p, const ButtonStates* in);
  void applyTickUpdates(GameScene* scene);
};
