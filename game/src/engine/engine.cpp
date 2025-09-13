/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

/*****************************
 ******** INPUT SYSTEM *******
 *****************************/
InputSystem::InputSystem() {}

// References [SDL docs](https://wiki.libsdl.org/SDL3/BestKeyboardPractices)
void InputSystem::updateButtonStates(const SDL_Event *e) {
  if (e->type == SDL_EVENT_KEY_DOWN) {
      if (e->key.scancode == SDL_SCANCODE_W && !e->key.repeat) { buttons.up = true; }
      else if (e->key.scancode == SDL_SCANCODE_S) { buttons.down = true; }
      else if (e->key.scancode == SDL_SCANCODE_A) { buttons.left = true; }
      else if (e->key.scancode == SDL_SCANCODE_D) { buttons.right = true; }
      else if (e->key.scancode == SDL_SCANCODE_U && !e->key.repeat) {buttons.b1 = true; }
      else if (e->key.scancode == SDL_SCANCODE_I && !e->key.repeat) {buttons.b2 = true; }
      else if (e->key.scancode == SDL_SCANCODE_J && !e->key.repeat) {buttons.b3 = true; }
      else if (e->key.scancode == SDL_SCANCODE_K && !e->key.repeat) {buttons.b4 = true; }
    }
  else if (e->type == SDL_EVENT_KEY_UP)
  {
    if (e->key.scancode == SDL_SCANCODE_W) { buttons.up = false; }
    else if (e->key.scancode == SDL_SCANCODE_S) { buttons.down = false; }
    else if (e->key.scancode == SDL_SCANCODE_A) { buttons.left = false; }
    else if (e->key.scancode == SDL_SCANCODE_D) { buttons.right = false; }
    else if (e->key.scancode == SDL_SCANCODE_U && !e->key.repeat) {buttons.b1 = false; }
    else if (e->key.scancode == SDL_SCANCODE_I && !e->key.repeat) {buttons.b2 = false; }
    else if (e->key.scancode == SDL_SCANCODE_J && !e->key.repeat) {buttons.b3 = false; }
    else if (e->key.scancode == SDL_SCANCODE_K && !e->key.repeat) {buttons.b4 = false; }
  }
}

/*****************************
 ******* GAME ALLOCATOR ******
 *****************************/
GameAllocator::GameAllocator() {
  cur_tick = 0;
  net_pindex = 99;
}

GameAllocator::GameAllocator(unsigned int net_p1_or_p2) {
  cur_tick = 0;
  net_pindex = net_p1_or_p2;
}

GameScene* GameAllocator::getCurrentScene() {
  return &history_buffer[getCurrentIndex()];
}

GameScene* GameAllocator::getNextScene() {
  const GameScene* prev_scene = getCurrentScene();
  cur_tick++;
  GameScene* new_scene = getCurrentScene();
  memcpy(new_scene, prev_scene, sizeof(GameScene));
  return new_scene;
}

unsigned int GameAllocator::getCurrentIndex() {
  return cur_tick % MAX_ROLLBACK_FRAMES;
}

GameScene* GameAllocator::rollBack(unsigned int prev_tick, const ButtonStates* in) {
  if (cur_tick - prev_tick > MAX_ROLLBACK_FRAMES) {
    std::cerr << "Error: Allocator recieved request for " << cur_tick - prev_tick
    << "f rollback" << std::endl
    << "  cur_tick: " << cur_tick << std::endl
    << "  prev_tick: " << prev_tick << std::endl;
  }
  if (net_pindex > 2) {
    std::cerr << "Error: Rollback requested when allocator initialized with no defined"
    << " online player" << std::endl;
  }
  cur_tick = prev_tick;
  memcpy(&history_buffer[getCurrentIndex()].inputs[net_pindex], in, sizeof(ButtonStates));
  return getCurrentScene();  
}

/*****************************
 ******** GAME MANAGER *******
 *****************************/
GameManager::GameManager() {
  // p1 = PlayerEntity();
  // p1_inputs = InputSystem();
}

GameManager::GameManager(unsigned int net_p1_or_p2) {
  // p1 = PlayerEntity();
  // p1_inputs = InputSystem();
  net_pindex = net_p1_or_p2;
  allocator = GameAllocator(net_p1_or_p2);
}

void GameManager::updateLocalInputs(SDL_Event* e) {
  local_inputs.updateButtonStates(e);

  GameScene* cur_scene = allocator.getCurrentScene();
  if (!cur_scene) {
    std::cerr << "Game allocator returned null scene to input update request\n";
  }

  memcpy(&cur_scene->inputs[0], &local_inputs.buttons, sizeof(ButtonStates));
}

void GameManager::setActions(PlayerEntity* p, const ButtonStates* in) {
  // Jump if on ground
  if (in->up && p->y_pos + p->height >= ENV_DIM_FLOOR_HEIGHT) {
    p->y_vel = p->jumping_v;
  }

  // Fast fall if in the air
  if (in->down && p->y_pos + p->height < ENV_DIM_FLOOR_HEIGHT) {
    p->y_vel += p->fastfall_v;
  }

  // Walk left if stopped
  if (in->left && p->x_vel <= 0) {
    p->x_vel = -p->walking_v;
  }

  // Walk right if stopped
  if (in->right && p->x_vel >= 0) {
    p->x_vel = p->walking_v;
  }
}

void GameManager::applyMovement(PlayerEntity* p) {
  // Don't collide left wall
  if (p->x_pos + p->x_vel < ENV_DIM_WALL_THICKNESS) {
    p->x_vel = ENV_DIM_WALL_THICKNESS - p->x_pos;
  }

  // Don't collide right wall
  if (p->x_pos + p->x_vel + p->width > ENV_DIM_RIGHT_WALL_X) {
    p->x_vel = ENV_DIM_RIGHT_WALL_X - (p->x_pos + p->width);
  }

  // Don't collide floor
  if (p->y_pos + p->y_vel + p->height > ENV_DIM_FLOOR_HEIGHT) {
    p->y_vel = ENV_DIM_FLOOR_HEIGHT - (p->y_pos + p->height);
  }

  // Apply velocities
  p->x_pos += p->x_vel;
  p->y_pos += p->y_vel;

  // Gravity
  if (p->y_pos + p->height < ENV_DIM_FLOOR_HEIGHT) {
    p->y_vel += p->gravity;
  }

  // Friction
  if (p->x_vel < 0) {
    p->x_vel += p->friction;
  } else if (p->x_vel > 0) {
    p->x_vel -= p->friction;
  }
}

void GameManager::updateFrames(PlayerEntity* p, const ButtonStates* in) {
  int startup_frames = 10;
  int active_frames = 10;
  int recovery_frames = 10;
  // Attack
  // TODO: Add dictionary of attacks
  if (in->b1 && (p->f_startup + p->f_active + p->f_recovery) == 0) {
    p->f_startup = startup_frames;
  }

  // Startup
  if (p->f_startup > 0) {
    // Decrement and check
    p->f_startup--;
    if (p->f_startup == 0) { 
      p->f_active = active_frames;
    }
  }

  // Active
  if (p->f_active > 0) {
    // Decrement and check
    p->f_active--;
    if (p->f_active == 0) { 
      p->f_recovery = recovery_frames;
    }
  }

  // Recovery
  if (p->f_recovery > 0) {
    p->f_recovery--;
  }
}

void GameManager::tick() {
  GameScene* cur_scene = allocator.getNextScene();
  setActions(&cur_scene->players[0], &cur_scene->inputs[0]);
  setActions(&cur_scene->players[1], &cur_scene->inputs[1]);
  updateFrames(&cur_scene->players[0], &cur_scene->inputs[0]);
  updateFrames(&cur_scene->players[1], &cur_scene->inputs[1]);
  applyMovement(&cur_scene->players[0]);
  applyMovement(&cur_scene->players[1]);
}

// Game Manager Getters

PlayerEntity* GameManager::getP1() {
  return &allocator.getCurrentScene()->players[0];
}

PlayerEntity* GameManager::getP2() {
  return &allocator.getCurrentScene()->players[1];
}
