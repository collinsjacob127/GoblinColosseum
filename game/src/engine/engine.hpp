/**
 * The input system is an abstraction layer between system input and input commands
 * 
 * Based on input system designed by rcmagic (wrote GGST netcode)
 * https://github.com/rcmagic/DemoFighterWithNetcode/blob/master/game/InputSystem.lua
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

  float width = 50.0;
  float height = 100.0;

  float walking_v = 15.0;
  float jumping_v = -15.0;
  float fastfall_v = 3.0;

  int f_startup = 0;
  int f_active = 0;
  int f_recovery = 0;

  float gravity = 0.5;
  float friction = 5.0;
};

// TODO: Implement rollback :)
// struct GameScene {
//   PlayerEntity p1;  
//   PlayerEntity p2;  
// };

// class GameAllocator {
//  public: 
//   getCurrentScene();

//  private:
//   GameScene history_buffer[MAX_ROLLBACK_FRAMES];
//   void getIndex();
// };

class GameManager {
 public:
  // PlayerBase p1_base;
  PlayerEntity p1;
  InputSystem p1_inputs;

  GameManager();

  // Sends inputs from SDL_Event to InputSystem
  void updateLocalInputs(SDL_Event* e);
  void setActions(PlayerEntity* p, const ButtonStates* in);
  void applyMovement(PlayerEntity* p);
  void tick();

 private:
  void updateFrames(PlayerEntity* p, const ButtonStates* in);
};
