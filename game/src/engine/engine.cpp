/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

InputSystem::InputSystem() {}

// References [SDL docs](https://wiki.libsdl.org/SDL3/BestKeyboardPractices)
void InputSystem::updateButtonStates(const SDL_Event *e) {
  if (e->type == SDL_EVENT_KEY_DOWN) {
      if (e->key.scancode == SDL_SCANCODE_W) { buttons.up = true; }
      else if (e->key.scancode == SDL_SCANCODE_S) { buttons.down = true; }
      else if (e->key.scancode == SDL_SCANCODE_A) { buttons.left = true; }
      else if (e->key.scancode == SDL_SCANCODE_D) { buttons.right = true; }
      else if (e->key.scancode == SDL_SCANCODE_SPACE && !e->key.repeat) {buttons.b1 = true; }
    }
  else if (e->type == SDL_EVENT_KEY_UP)
  {
    if (e->key.scancode == SDL_SCANCODE_W) { buttons.up = false; }
    else if (e->key.scancode == SDL_SCANCODE_S) { buttons.down = false; }
    else if (e->key.scancode == SDL_SCANCODE_A) { buttons.left = false; }
    else if (e->key.scancode == SDL_SCANCODE_D) { buttons.right = false; }
    else if (e->key.scancode == SDL_SCANCODE_SPACE) {buttons.b1 = false; }
  }
}

GameManager::GameManager() {
  // p1 = PlayerEntity();
  // p1_inputs = InputSystem();
}

void GameManager::updateLocalInputs(SDL_Event* e) {
  p1_inputs.updateButtonStates(e);
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
  setActions(&p1, &p1_inputs.buttons);
  updateFrames(&p1, &p1_inputs.buttons);
  applyMovement(&p1);
}
