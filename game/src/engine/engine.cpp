/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

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
  startup = 50;
  active = 100;
  recovery = 50;

  x_offset = 100;
  y_offset = 25;
  x_width = 50;
  y_width = 30;

  name = "TEMP_ATTACK";

  damage = 5;
  hit_vel_x = 10.0;
  hit_vel_y = -5.0;
}

PlayerState::PlayerState() {
  width = 100;
  height = 150;

  x_pos = 480;
  y_pos = 880-height;

  x_vel = 0;
  y_vel = 0;

  x_acc = 0;
  y_acc = 0;

  walking_speed = 10;
  jumping_v0 = -15;
  fastfall_v = 2;

  startup_frames = 0;
  active_frames = 0;
  recovery_frames = 0;
}

void PlayerState::processInputs(const InputState *inputs) {
  // Handle movement requests
  // Only jump if on the ground
  if (inputs->up && y_pos + height >= ENV_DIM_FLOOR_HEIGHT) { y_vel = jumping_v0; }
  // fastfall
  if (inputs->down) { y_vel += fastfall_v; }
  // walk left / right
  if (inputs->left && x_vel <= 0) { x_vel = -walking_speed; }
  if (inputs->right && x_vel >= 0) { x_vel = walking_speed; }

  // Handle action requests
  if (inputs->attack && (startup_frames + active_frames + recovery_frames == 0)) {
    startup_frames = attack->startup;
  }
}

void PlayerState::computeNextState() {
  // Apply movement
  // Don't collide left
  if (x_pos + x_vel < ENV_DIM_WALL_THICKNESS) {
    x_vel = ENV_DIM_WALL_THICKNESS - x_pos;
  } 

  // Don't collide right
  if (x_pos + x_vel + width > ENV_DIM_RIGHT_WALL_X) {
    x_vel = ENV_DIM_RIGHT_WALL_X - (x_pos + width);
  } 
  x_pos += x_vel;

  // Don't collide down
  if (y_pos + y_vel + height > ENV_DIM_FLOOR_HEIGHT) {
    y_vel = ENV_DIM_FLOOR_HEIGHT - (y_pos + height);
  }
  // apply velocity
  y_pos += y_vel;

  // gravity
  if (y_pos + height < ENV_DIM_FLOOR_HEIGHT) {
    y_vel++;
  }

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
      active_frames = attack->active;
    }
    return;
  }

  // Decrement active frames
  if (active_frames > 0) {
    active_frames--; 
    if (active_frames == 0) {
      recovery_frames = attack->recovery;
    }
    return;
  }

  // Decrement recovery frames
  if (recovery_frames > 0) {
    recovery_frames--;
    return;
  }
}

// Needs work
bool PlayerState::computeCollision(const PlayerState *opp) {
  if (active_frames == 0) {
    return false;
  }

  float atk_low_x = x_pos + attack->x_offset;
  float atk_high_x = x_pos + attack->x_offset + attack->x_width;
  float atk_low_y = y_pos + attack->y_offset;
  float atk_high_y = y_pos + attack->y_offset + attack->y_width;

  float opp_low_x = 0;
  float opp_high_x = 0;
  float opp_low_y = 0;
  float opp_high_y = 0;

  // if (attack->x_offset + x_pos) {
  //   return true;
  // }
  return false;
}

void PlayerState::copyFrom(const PlayerState *src) {
  width = src->width;
  height = src->height;

  x_pos = src->x_pos;
  y_pos = src->y_pos;

  x_vel = src->x_vel;
  y_vel = src->y_vel;

  startup_frames = src->startup_frames;
  active_frames = src->active_frames;
  recovery_frames = src->recovery_frames;
}

GameScene::GameScene() {
  merged = false;
  cur_tick = 0;
};

GameScene::GameScene(const PlayerState* p1, const InputState* i1, const PlayerState* p2, const InputState* i2) {
  player1 = *p1;
  inputs1 = *i1;

  player2 = *p2;
  inputs2 = *i2;

  merged = false;
  unsigned int cur_tick = 0;
}

void GameScene::copyFrom(const GameScene* src) {
  player1.copyFrom(&src->player1);
  player2.copyFrom(&src->player2);
  // Copy inputs
}

GameManager::GameManager() {
  // Initialize scene list
  for (unsigned int i = 0; i < MAX_ROLLBACK_FRAMES; ++i) {
    scenes[i] = GameScene();
  }
  cur_scene_index = 0;
  total_ticks = 0;
}

/**
 * @brief Calculate the next game state and overwrite next index with it
 */
bool GameManager::tick(InputState p1_inputs, InputState p2_inputs) {
  // TODO:
  // Copy current frame to next

  // Update total_ticks
  total_ticks++;
  // Update cur_scene_index
  cur_scene_index = next_index();

  return true;
}

unsigned int GameManager::next_index() {
  return (cur_scene_index + 1) % MAX_ROLLBACK_FRAMES;
}

