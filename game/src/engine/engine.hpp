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
#include <string.h>
#include <SDL3/SDL.h>

#define MAX_ROLLBACK_FRAMES 60
#define FRAME_ADVANTAGE_LIMIT 5
#define INITIAL_FRAME 0

#define MAX_INPUT_FRAMES 60

#define ENV_DIM_WALL_THICKNESS 20
#define ENV_DIM_FLOOR_HEIGHT 880
#define ENV_DIM_LEFT_WALL_X 0
#define ENV_DIM_RIGHT_WALL_X 1900

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

class InputSystem {
 public:
  ButtonStates buttons;

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
};

// TODO: Implement rollback :)
struct GameScene {
  PlayerEntity p1;  
  PlayerEntity p2;  
  ButtonStates in1;
  ButtonStates in2;
};

class GameAllocator {
 public: 
  unsigned int cur_tick;

  GameAllocator();

  GameScene* getCurrentScene();

  /**
   * @brief Function to move on to the next scene.
   * Copies current scene to next index, increments cur_tick,
   * and returns the *new* current scene.
   */
  GameScene* getNextScene();

 private:
  GameScene history_buffer[MAX_ROLLBACK_FRAMES];
  unsigned int getCurrentIndex();
};

class GameManager {
 public:
  // PlayerBase p1_base;
  GameAllocator game_allocator;
  InputSystem local_inputs;

  GameManager();

  // Sends inputs from SDL_Event to InputSystem
  void updateLocalInputs(SDL_Event* e);
  void setActions(PlayerEntity* p, const ButtonStates* in);
  void applyMovement(PlayerEntity* p);
  void tick();

  // getters
  PlayerEntity* getP1();
  PlayerEntity* getP2();

 private:
  void updateFrames(PlayerEntity* p, const ButtonStates* in);
};
