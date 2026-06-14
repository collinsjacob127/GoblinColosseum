/**
 * Author: Jacob Collins
 * Description: Definitions for button mapping and 
 * button/input state classes
 */

#include "buttons.hpp"

/*****************************
 ******** INPUT SYSTEM *******
 *****************************/
InputSystem::InputSystem() {
  for (unsigned int i = 0; i < MAX_INPUT_FRAMES; ++i) {
    buttons.dir_buffer[i] = CENTER;
  }
}

void applyButtonUpdate(const Button* prev_btn, Button* cur_btn) {
  // Button currently held
  if (*cur_btn >= 1) {
    // Previous button not held
    if (*prev_btn <= 0) { *cur_btn = PRESSED; }
    // Previous button held
    else { *cur_btn = HELD; }
  } else { // Button currently NOT held
    // Previous button not held
    if (*prev_btn <= 0) { *cur_btn = RELEASED; }
    // Previous button held
    else { *cur_btn = JUST_RELEASED; }
  }
}

void handleButtonStateTick(const ButtonStates* prev_buttons, ButtonStates* cur_buttons) {
  applyButtonUpdate(&prev_buttons->up, &cur_buttons->up); 
  applyButtonUpdate(&prev_buttons->down, &cur_buttons->down); 
  applyButtonUpdate(&prev_buttons->left, &cur_buttons->left); 
  applyButtonUpdate(&prev_buttons->right, &cur_buttons->right); 
  applyButtonUpdate(&prev_buttons->b1, &cur_buttons->b1); 
  applyButtonUpdate(&prev_buttons->b2, &cur_buttons->b2); 
  applyButtonUpdate(&prev_buttons->b3, &cur_buttons->b3); 
  applyButtonUpdate(&prev_buttons->b4, &cur_buttons->b4); 
  applyButtonUpdate(&prev_buttons->l1, &cur_buttons->l1); 
  applyButtonUpdate(&prev_buttons->l2, &cur_buttons->l2); 
  applyButtonUpdate(&prev_buttons->r1, &cur_buttons->r1); 
  applyButtonUpdate(&prev_buttons->r2, &cur_buttons->r2); 
}

// References [SDL docs](https://wiki.libsdl.org/SDL3/BestKeyboardPractices)
void InputSystem::updateButtonStates(const SDL_Event *e) {
  if (e->type == SDL_EVENT_KEY_DOWN) {
    if (e->key.scancode == bindings.up) { buttons.up = PRESSED; }
    else if (e->key.scancode == bindings.down) { buttons.down = PRESSED; }
    else if (e->key.scancode == bindings.left) { buttons.left = PRESSED; }
    else if (e->key.scancode == bindings.right) { buttons.right = PRESSED; }
    else if (e->key.scancode == bindings.b1 ) {buttons.b1 = PRESSED; }
    else if (e->key.scancode == bindings.b2 ) {buttons.b2 = PRESSED; }
    else if (e->key.scancode == bindings.b3) {buttons.b3 = PRESSED; }
    else if (e->key.scancode == bindings.b4) {buttons.b4 = PRESSED; }
    else if (e->key.scancode == bindings.l1) {buttons.l1 = PRESSED; }
    else if (e->key.scancode == bindings.r1) {buttons.r1 = PRESSED; }
    else if (e->key.scancode == bindings.l2) {buttons.l2 = PRESSED; }
    else if (e->key.scancode == bindings.r2) {buttons.r2 = PRESSED; }
  } else if (e->type == SDL_EVENT_KEY_UP) {
    if (e->key.scancode == bindings.up) { buttons.up = RELEASED; }
    else if (e->key.scancode == bindings.down) { buttons.down = RELEASED; }
    else if (e->key.scancode == bindings.left) { buttons.left = RELEASED; }
    else if (e->key.scancode == bindings.right) { buttons.right = RELEASED; }
    else if (e->key.scancode == bindings.b1) {buttons.b1 = RELEASED; }
    else if (e->key.scancode == bindings.b2) {buttons.b2 = RELEASED; }
    else if (e->key.scancode == bindings.b3) {buttons.b3 = RELEASED; }
    else if (e->key.scancode == bindings.b4) {buttons.b4 = RELEASED; }
    else if (e->key.scancode == bindings.l1) {buttons.l1 = RELEASED; }
    else if (e->key.scancode == bindings.r1) {buttons.r1 = RELEASED; }
    else if (e->key.scancode == bindings.l2) {buttons.l2 = RELEASED; }
    else if (e->key.scancode == bindings.r2) {buttons.r2 = RELEASED; }
  }
}

std::string printButtonState(const Button* btn) {
  switch (*btn) {
    case (PRESSED): { return "PRESSED"; }
    case (HELD): { return "HELD"; }
    case (RELEASED): { return "RELEASED"; }
    default: {return "NULL";}
  }
}

void showButtonStates(const ButtonStates* btn_state) {
  std::cout << "Buttons: ";
  if (btn_state->up) {std::cout << " up_" << printButtonState(&btn_state->up);}
  if (btn_state->down) {std::cout << " down_" << printButtonState(&btn_state->down);}
  if (btn_state->left) {std::cout << " left_" << printButtonState(&btn_state->left);}
  if (btn_state->right) {std::cout << " right_" << printButtonState(&btn_state->right);}
  if (btn_state->b1) {std::cout << " b1_" << printButtonState(&btn_state->b1);}
  if (btn_state->b2) {std::cout << " b2_" << printButtonState(&btn_state->b2);}
  if (btn_state->b3) {std::cout << " b3_" << printButtonState(&btn_state->b3);}
  if (btn_state->b4) {std::cout << " b4_" << printButtonState(&btn_state->b4);}
  if (btn_state->l1) {std::cout << " l1_" << printButtonState(&btn_state->l1);}
  if (btn_state->l2) {std::cout << " l2_" << printButtonState(&btn_state->l2);}
  if (btn_state->r1) {std::cout << " r1_" << printButtonState(&btn_state->r1);}
  if (btn_state->r2) {std::cout << " r2_" << printButtonState(&btn_state->r2);}
  std::cout << std::endl;
}

bool isAnyButtonPressed(const ButtonStates* in) {
  return in->b1 || in->b2 || in->b3 || in->b4 || in->l1 || in->l2 || in->r1 || in->r2;
}

Button getButtonState(const ButtonStates* in, ButtonName b) {
  switch (b) {
    case (B1): { return in->b1; }
    case (B2): { return in->b2; }
    case (B3): { return in->b3; }
    case (B4): { return in->b4; }
    case (L1): { return in->l1; }
    case (R1): { return in->r1; }
    case (L2): { return in->l2; }
    case (R2): { return in->r2; }
    default: return RELEASED;
  }
}

void InputSystem::resetButtonStates() {
  buttons = ButtonStates();
}

std::string getNumPadDirString(const NumPadDir* dir) {
  switch (*dir) {
    case (DOWN_LEFT): { return "DL"; }
    case (DOWN): {return "D"; }
    case (DOWN_RIGHT): {return "DR";}
    case (LEFT): {return "L";}
    case (CENTER): {return "C";}
    case (RIGHT): {return "R";}
    case (UP_LEFT): {return "UL";}
    case (UP): {return "U";}
    case (UP_RIGHT): {return "UR";}
    default: return " ";
  }
}

void printMotionBuffer(NumPadDir* dir_buffer) {
  for (unsigned int i = 0; i < MAX_INPUT_FRAMES; ++i) {
    std::cout << getNumPadDirString(&dir_buffer[i]) << " ";
  }
  std::cout << std::endl;
}

void InputSystem::setP2DefaultBindings() {
  bindings.up = SDL_SCANCODE_UP;
  bindings.down = SDL_SCANCODE_DOWN;
  bindings.left = SDL_SCANCODE_LEFT;
  bindings.right = SDL_SCANCODE_RIGHT;
  bindings.b1 = SDL_SCANCODE_KP_7; // ps square
  bindings.b2 = SDL_SCANCODE_KP_8; // ps triangle
  bindings.b3 = SDL_SCANCODE_KP_9; // ps X
  bindings.b4 = SDL_SCANCODE_KP_PLUS; // ps circle
  bindings.l1 = SDL_SCANCODE_KP_4; // left bumper
  bindings.r1 = SDL_SCANCODE_KP_5; // right bumper
  bindings.l2 = SDL_SCANCODE_KP_1; // left trigger
  bindings.r2 = SDL_SCANCODE_KP_2; // right trigger
}
