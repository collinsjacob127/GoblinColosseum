/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

void test_engine_include_works() {
  std::cout << "Engine include works!\n";
}

/* Input State
 * Tracks the boolean value of each input
 */
InputState::InputState() {
  up = false;
  down = false;
  left = false;
  right = false;
  attack = false;
}

void InputState::reset() {
  up = false;
  down = false;
  left = false;
  right = false;
  attack = false;
}

InputState InputState::copy() {
  InputState cur_state;
  cur_state.up = up;
  cur_state.down = down;
  cur_state.left = left;
  cur_state.right = right;
  cur_state.attack = attack;
  return cur_state;
}

InputSystem::InputSystem() {
  inputDelay = 0;
}

// References [SDL docs](https://wiki.libsdl.org/SDL3/BestKeyboardPractices)
void InputSystem::updateInputState(const SDL_Event *e) {
  if (e->type == SDL_EVENT_KEY_DOWN) {
      if (e->key.scancode == SDL_SCANCODE_W) { inputState.up = true; }
      else if (e->key.scancode == SDL_SCANCODE_S) { inputState.down = true; }
      else if (e->key.scancode == SDL_SCANCODE_A) { inputState.left = true; }
      else if (e->key.scancode == SDL_SCANCODE_D) { inputState.right = true; }
      else if (e->key.scancode == SDL_SCANCODE_SPACE && !e->key.repeat) {inputState.attack = true; }
    }
  else if (e->type == SDL_EVENT_KEY_UP)
  {
    if (e->key.scancode == SDL_SCANCODE_W) { inputState.up = false; }
    else if (e->key.scancode == SDL_SCANCODE_S) { inputState.down = false; }
    else if (e->key.scancode == SDL_SCANCODE_A) { inputState.left = false; }
    else if (e->key.scancode == SDL_SCANCODE_D) { inputState.right = false; }
    else if (e->key.scancode == SDL_SCANCODE_SPACE) {inputState.attack = false; }
  }
}

PlayerAction::PlayerAction() {
  startup = 10;
  active = 15;
  recovery = 15;
  x_offset = 100;
  y_offset = 25;
  x_width = 50;
  y_width = 30;
  name = "TEMP_ATTACK";
}

PlayerState::PlayerState() {
  x_pos = 100;
  y_pos = 100;

  x_vel = 0;
  y_vel = 0;

  x_acc = 0;
  y_acc = 0;

  walking_speed = 10;

  startup_frames = 0;
  active_frames = 0;
  recovery_frames = 0;
}

void PlayerState::processInputs(const InputState *inputs) {
  // Handle movement requests
  if (inputs->up) { y_pos -= walking_speed; }
  if (inputs->down) { y_pos += walking_speed; }
  if (inputs->left && x_vel <= 0) { x_vel = -walking_speed; }
  if (inputs->right && x_vel >= 0) { x_vel = walking_speed; }

  // Handle action requests
  if (inputs->attack && (startup_frames + active_frames + recovery_frames == 0)) {
    startup_frames = attack.startup;
  }
}

void PlayerState::computeNextState() {
  // Apply movement
  x_pos += x_vel;

  // Apply friction (gradual stop)
  if (x_vel < 0) {
    x_vel++;
  } else if (x_vel > 0) {
    x_vel--;
  }

  // Update attack
  // Startup
  if (startup_frames > 0) {
    startup_frames--;
    // At end of startup frames, begin attack frames
    if (startup_frames == 0) {
      active_frames = attack.active;
    }
    return;
  }

  // Decrement active frames
  if (active_frames > 0) {
    active_frames--; 
    if (active_frames == 0) {
      recovery_frames = attack.recovery;
    }
    return;
  }

  // Decrement recovery frames
  if (recovery_frames > 0) {
    recovery_frames--;
    return;
  }
}