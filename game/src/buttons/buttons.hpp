/**
 * Author: Jacob Collins
 * Description: Headers for button mapping and 
 * button/input state classes
 */

#pragma once

#include<SDL3/SDL.h>
#include "util.hpp"

#define MAX_INPUT_FRAMES 60

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

enum Button {
  PRESSED = 2, // Pressed this frame
  HELD = 1,    // Still being held (also basic held, for binary scenarios)
  RELEASED = 0, // Un-Held (also basic NOT held, for binary scenarios)
  JUST_RELEASED = -1 // JUST released
};

enum NumPadDir {
  DOWN_LEFT = 1,
  DOWN = 2,
  DOWN_RIGHT = 3,
  LEFT = 4,
  CENTER = 5,
  RIGHT = 6,
  UP_LEFT = 7,
  UP = 8,
  UP_RIGHT = 9,
  MISC = 10,
};

enum ButtonName {
  B1,
  B2,
  B3,
  B4,
  L1,
  R1,
  L2,
  R2
};

struct ButtonStates {
  Button up = RELEASED;
  Button down = RELEASED;
  Button left = RELEASED;
  Button right = RELEASED;
  // PS square
  Button b1 = RELEASED; 
  // ps triangle
  Button b2 = RELEASED; 
  // ps X
  Button b3 = RELEASED; 
  // ps circle
  Button b4 = RELEASED; 
  Button l1 = RELEASED;
  Button r1 = RELEASED;
  Button l2 = RELEASED;
  Button r2 = RELEASED;
  // Not yet bound
  Button l3 = RELEASED;
  Button r3 = RELEASED;

  // Not sent
  Button start = RELEASED;
  Button select = RELEASED;

  NumPadDir dir_buffer[MAX_INPUT_FRAMES];
};

// Used for debugging
std::string printButtonState(const Button* btn);
void printMotionBuffer(NumPadDir* dir_buffer);
void showButtonStates(const ButtonStates* btn_state);
bool isAnyButtonPressed(const ButtonStates* in);
Button getButtonState(const ButtonStates* in, ButtonName b);

class InputSystem {
 public:
  ButtonStates buttons;
  Keybinds bindings;

  InputSystem();
  // Directly sets buttons to pressed or unpressed in the current allocation
  void updateButtonStates(const SDL_Event *e);
  void resetButtonStates();
  void setP2DefaultBindings();
 private:
};

void applyButtonUpdate(const Button* prev_btn, Button* cur_btn);

/** 
 * @brief Differentiates between pressed and held button states based on previous state
 * @param prev_buttons Pointer to the reference button state
 * @param cur_buttons Pointer to the current button state, to be updated based on the previous.
 * @note Updates current tick's button state based on the previous button state.
 * ONLY USE ONCE PER TICK
 */
void handleButtonStateTick(const ButtonStates* prev_buttons, ButtonStates* cur_buttons);
