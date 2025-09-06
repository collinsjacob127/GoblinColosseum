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
  int startup;
  int active;
  int recovery;
  float x_offset;
  float y_offset;
  float x_width;
  float y_width;
  std::string name;

  PlayerAction();
};

class PlayerState {
 public: 
  float x_pos;
  float y_pos;
  int x_vel;
  int y_vel;
  int x_acc;
  int y_acc;

  PlayerAction attack;

  // Walking speed in px / frame
  int walking_speed;

  int startup_frames;
  int active_frames;
  int recovery_frames;

  PlayerState();

  void processInputs(const InputState *inputs);
  void computeNextState();
};
