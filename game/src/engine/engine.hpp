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

#define MAX_INPUT_FRAMES 60

#define ENV_DIM_WALL_THICKNESS 20
#define ENV_DIM_FLOOR_HEIGHT 880
#define ENV_DIM_LEFT_WALL_X 0
#define ENV_DIM_RIGHT_WALL_X 1900

void test_engine_include_works();

class InputState {
 public:
  bool up;
  bool down;
  bool left;
  bool right;
  bool attack;

  InputState();

  void reset();

  InputState copy();
};

class InputSystem {
 public:
  // uint MAX_INPUT_FRAMES = 60;

  // uint local_player_index = 1;
  // uint local_player_index = 2;

  InputState inputState;

  InputState remotePlayerState;

  // polledInput;

  // InputState playerCommandBuffer[MAX_INPUT_FRAMES];

  uint inputDelay;

  InputSystem();

  void updateInputState(const SDL_Event *e);
};

class PlayerAction {
 public:
  // Frames
  int startup;
  int active;
  int recovery;

  // Hitboxes
  float x_offset;
  float y_offset;
  float x_width;
  float y_width;

  // DMG Values
  float damage;
  float hit_vel_x;
  float hit_vel_y;

  // Tags
  std::string name;

  PlayerAction();
};

class PlayerState {
 public: 
  float width;
  float height;

  float x_pos;
  float y_pos;

  int x_vel;
  int y_vel;

  int x_acc;
  int y_acc;

  PlayerAction attack;

  // Walking speed in px / frame
  int walking_speed;
  int jumping_v0;
  float fastfall_v;

  int startup_frames;
  int active_frames;
  int recovery_frames;

  PlayerState();

  void processInputs(const InputState *inputs);
  void computeNextState();
  bool computeCollision(const PlayerState *opp);
};

