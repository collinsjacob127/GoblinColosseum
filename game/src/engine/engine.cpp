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

void InputState::copyFrom(const InputState *src) {
  up = src->up;
  down = src->down;
  left = src->left;
  right = src->right;
  attack = src->attack;
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

PlayerBase::PlayerBase() {
  width = 100;
  height = 150;

  walking_speed = 10;
  jumping_v0 = -15;
  fastfall_v = 2;
}

PlayerState::PlayerState(const PlayerBase* base) {
  x_pos = 480;
  y_pos = 880-(base->height);

  x_vel = 0;
  y_vel = 0;

  x_acc = 0;
  y_acc = 0;

  startup_frames = 0;
  active_frames = 0;
  recovery_frames = 0;
}

void PlayerState::copyFrom(const PlayerState *src) {
  x_pos = src->x_pos;
  y_pos = src->y_pos;

  x_vel = src->x_vel;
  y_vel = src->y_vel;

  x_acc = src->x_acc;
  y_acc = src->y_acc;

  startup_frames = src->startup_frames;
  active_frames = src->active_frames;
  recovery_frames = src->recovery_frames;
}


void PlayerManager::processInputs(const InputState *inputs, PlayerState* state) {
  // Handle movement requests
  // Only jump if on the ground
  if (inputs->up && state->y_pos + base->height >= ENV_DIM_FLOOR_HEIGHT) { state->y_vel = base->jumping_v0; }
  // fastfall
  if (inputs->down) { state->y_vel += base->fastfall_v; }
  // walk left / right
  if (inputs->left && state->x_vel <= 0) { state->x_vel = -base->walking_speed; }
  if (inputs->right && state->x_vel >= 0) { state->x_vel = base->walking_speed; }

  // Handle action requests
  if (inputs->attack && (state->startup_frames + state->active_frames + state->recovery_frames == 0)) {
    state->startup_frames = base->attack->startup;
  }
}

void PlayerManager::computeNextState(PlayerState* state) {
  // Apply movement
  // Don't collide left
  if (state->x_pos + state->x_vel < ENV_DIM_WALL_THICKNESS) {
    state->x_vel = ENV_DIM_WALL_THICKNESS - state->x_pos;
  } 

  // Don't collide right
  if (state->x_pos + state->x_vel + base->width > ENV_DIM_RIGHT_WALL_X) {
    state->x_vel = ENV_DIM_RIGHT_WALL_X - (state->x_pos + base->width);
  } 
  state->x_pos += state->x_vel;

  // Don't collide down
  if (state->y_pos + state->y_vel + base->height > ENV_DIM_FLOOR_HEIGHT) {
    state->y_vel = ENV_DIM_FLOOR_HEIGHT - (state->y_pos + base->height);
  }
  // apply velocity
  state->y_pos += state->y_vel;

  // gravity
  if (state->y_pos + base->height < ENV_DIM_FLOOR_HEIGHT) {
    state->y_vel++;
  }

  // Apply friction (gradual stop)
  if (state->x_vel < 0) {
    state->x_vel++;
  } else if (state->x_vel > 0) {
    state->x_vel--;
  }

  // Update attack
  // Startup
  if (state->startup_frames > 0) {
    state->startup_frames--;
    // At end of startup frames, begin attack frames
    if (state->startup_frames == 0) {
      state->active_frames = base->attack->active;
    }
    return;
  }

  // Decrement active frames
  if (state->active_frames > 0) {
    state->active_frames--; 
    if (state->active_frames == 0) {
      state->recovery_frames = base->attack->recovery;
    }
    return;
  }

  // Decrement recovery frames
  if (state->recovery_frames > 0) {
    state->recovery_frames--;
    return;
  }
}

// Needs work
bool PlayerManager::computeCollision(PlayerState* state, PlayerState *opp_state) {
  if (state->active_frames == 0) {
    return false;
  }

  float atk_low_x = state->x_pos + base->attack->x_offset;
  float atk_high_x = state->x_pos + base->attack->x_offset + base->attack->x_width;
  float atk_low_y = state->y_pos + base->attack->y_offset;
  float atk_high_y = state->y_pos + base->attack->y_offset + base->attack->y_width;

  float opp_low_x = 0;
  float opp_high_x = 0;
  float opp_low_y = 0;
  float opp_high_y = 0;

  // if (attack->x_offset + state->x_pos) {
  //   return true;
  // }
  return false;
}

GameScene::GameScene(const PlayerBase* pbase1, const PlayerBase* pbase2) {
  player1 = PlayerState(pbase1);
  player2 = PlayerState(pbase2);
  merged = false;
  cur_tick = 0;
};

void GameScene::copyFrom(const GameScene* src) {
  // Copy player info
  player1.copyFrom(&src->player1);
  player2.copyFrom(&src->player2);

  // Copy inputs
  inputs1.copyFrom(&src->inputs1);
  inputs2.copyFrom(&src->inputs2);

  merged = false;

  // Increment tick
  cur_tick = src->cur_tick+1;
}

GameManager::GameManager() {
  // Initialize scene list
  for (unsigned int i = 0; i < MAX_ROLLBACK_FRAMES; ++i) {
    scenes[i] = GameScene();
  }
  cur_scene_index = 0;
  total_ticks = INITIAL_FRAME;
  last_merged_tick = INITIAL_FRAME;
}

/**
 * @brief Calculate the next game state and overwrite next index with it
 */
bool GameManager::tick(const InputState *p1_inputs, const InputState *p2_inputs, bool rb) {
  unsigned int new_index = next_index();

  // Copy current frame to next
  scenes[next_index()].copyFrom(&scenes[cur_scene_index]); 

  // Update total_ticks
  if (rb) {
    total_ticks++;
  }

  // Update cur_scene_index
  cur_scene_index = next_index();

  GameScene* scene = getCurrentScene();

  p1_mgr.processInputs(p1_inputs, &scene->player1);
  p2_mgr.processInputs(p2_inputs, &scene->player2);

  p1_mgr.computeNextState(&scene->player1);
  p2_mgr.computeNextState(&scene->player2);

  return true;
}

unsigned int GameManager::next_index() {
  return (cur_scene_index + 1) % MAX_ROLLBACK_FRAMES;
}

bool GameManager::rollback(const InputState *p2_inputs, unsigned int rb_tick) {
  return true;
}

GameScene* GameManager::getCurrentScene() {
  return &scenes[cur_scene_index];
}
