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
    if (e->key.scancode == bindings.up && !e->key.repeat) { buttons.up = true; }
    else if (e->key.scancode == bindings.down) { buttons.down = true; }
    else if (e->key.scancode == bindings.left) { buttons.left = true; }
    else if (e->key.scancode == bindings.right) { buttons.right = true; }
    else if (e->key.scancode == bindings.b1 && !e->key.repeat) {buttons.b1 = true; }
    else if (e->key.scancode == bindings.b2 && !e->key.repeat) {buttons.b2 = true; }
    else if (e->key.scancode == bindings.b3 && !e->key.repeat) {buttons.b3 = true; }
    else if (e->key.scancode == bindings.b4 && !e->key.repeat) {buttons.b4 = true; }
    else if (e->key.scancode == bindings.l1 && !e->key.repeat) {buttons.l1 = true; }
    else if (e->key.scancode == bindings.r1 && !e->key.repeat) {buttons.r1 = true; }
    else if (e->key.scancode == bindings.l2 && !e->key.repeat) {buttons.l2 = true; }
    else if (e->key.scancode == bindings.r2 && !e->key.repeat) {buttons.r2 = true; }
  } else if (e->type == SDL_EVENT_KEY_UP) {
    if (e->key.scancode == bindings.up) { buttons.up = false; }
    else if (e->key.scancode == bindings.down) { buttons.down = false; }
    else if (e->key.scancode == bindings.left) { buttons.left = false; }
    else if (e->key.scancode == bindings.right) { buttons.right = false; }
    else if (e->key.scancode == bindings.b1 && !e->key.repeat) {buttons.b1 = false; }
    else if (e->key.scancode == bindings.b2 && !e->key.repeat) {buttons.b2 = false; }
    else if (e->key.scancode == bindings.b3 && !e->key.repeat) {buttons.b3 = false; }
    else if (e->key.scancode == bindings.b4 && !e->key.repeat) {buttons.b4 = false; }
    else if (e->key.scancode == bindings.l1 && !e->key.repeat) {buttons.l1 = false; }
    else if (e->key.scancode == bindings.r1 && !e->key.repeat) {buttons.r1 = false; }
    else if (e->key.scancode == bindings.l2 && !e->key.repeat) {buttons.l2 = false; }
    else if (e->key.scancode == bindings.r2 && !e->key.repeat) {buttons.r2 = false; }
  }
}

void showButtonStates(const ButtonStates* btn) {
  // TODO: Display state of each button here.
}

/*****************************
 ******* GAME ALLOCATOR ******
 *****************************/
GameAllocator::GameAllocator() {
  cur_tick = 0;
  net_pindex = 99;
  loc_pindex = 0;
  if (ENABLE_HELPER_PRINTOUTS) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Frame buffer allocated: " << (float) sizeof(history_buffer)/1024 << " Kb\n";
  }
}

GameAllocator::GameAllocator(unsigned int net_p1_or_p2) {
  cur_tick = 0;
  net_pindex = net_p1_or_p2;
  // net and loc should always be opposites
  if (net_pindex == 0) { loc_pindex = 1;}
  else if (net_pindex == 1) { loc_pindex = 0;}
  else { 
    std::cerr << "Game allocator recieved invalid net_pindex (" << net_pindex
    << ") whats up with that\n";
    loc_pindex = 0;
  }
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

/**
 * @return The index at which the current frame's scene lies in `history_buffer`
 */
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
  // Update current tick label (marking that we've rolled back)
  cur_tick = prev_tick;
  // Copy recieved inputs where they should go
  memcpy(&history_buffer[getCurrentIndex()].inputs[net_pindex], in, sizeof(ButtonStates));
  // Returns the scene at that point in time
  return getCurrentScene();  
}

/**
 * @brief Copy state to the next position in the buffer, without affecting
 * the local player's existing inputs
 */
GameScene* GameAllocator::rollForward() {
  // Temporary buffer for local player's inputs in the next frame
  ButtonStates tmp;
  GameScene* cur_scene = getCurrentScene();
  cur_tick++;
  GameScene* next_scene = getCurrentScene();
  // Copy next frame's existing local inputs to the buffer
  memcpy(&tmp, &next_scene->inputs[loc_pindex], sizeof(ButtonStates));
  // Copy current frame into next frame
  memcpy(next_scene, cur_scene, sizeof(GameScene));
  // Copy saved inputs to next frame
  memcpy(&next_scene->inputs[loc_pindex], &tmp, sizeof(ButtonStates));
  return next_scene;
}

/*****************************
 ******** GAME MANAGER *******
 *****************************/
GameManager::GameManager() {
  cur_tick = 0;
  loc_pindex = 0;
  net_pindex = 1;
  allocator = GameAllocator(net_pindex+1);
}

GameManager::GameManager(unsigned int net_p1_or_p2) {
  cur_tick = 0;
  net_pindex = net_p1_or_p2;
  if (net_pindex == 0) { loc_pindex = 1; }
  else if (net_pindex == 1) { loc_pindex = 0; }
  else { std::cerr << "Invalid net pindex\n"; }

  allocator = GameAllocator(net_p1_or_p2);
}

/**
 * @brief Sends inputs from SDL_Event to InputSystem
 */
void GameManager::updateLocalInputs(SDL_Event* e) {
  inputs[loc_pindex].updateButtonStates(e);

  GameScene* cur_scene = allocator.getCurrentScene();
  if (!cur_scene) {
    std::cerr << "Game allocator returned null scene to input update request\n";
  }

  // Send inputs from 
  memcpy(&cur_scene->inputs[loc_pindex], 
         &inputs[loc_pindex].buttons, 
         sizeof(ButtonStates));
}

/**
 * @brief Set a player's actions given their button inputs
 */
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


/**
 * @brief Apply movement to a player using their velocities
 * @param p Pointer to the player's entity in the game scene
 * @note Handles collision
 */
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


/**
 * @brief Handles incrementation and state changes for actions
 * involving frame data (attacks, hitstun, etc.)
 */
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
  cur_tick++;
  GameScene* cur_scene = allocator.getNextScene();
  applyTickUpdates(cur_scene);
}

/**
 * @brief Roll the allocator back to a given frame, insert the recieved inputs,
 * then resimulate forwards until the current frame.
 */
void GameManager::rollBack(unsigned int frame, const ButtonStates* in) {
  // Move the current game scene back to when this input was sent
  // Send the input to the allocator
  allocator.rollBack(frame, in);
  GameScene* scene;
  // Iterate through previous frames until the present
  for (unsigned int i = frame; i < cur_tick; ++i) {
    scene = allocator.rollForward();
    applyTickUpdates(scene);
  }
}

/**
 * @brief Private helper functions to apply all game logic to a scene
 * based on the previous scene's state.
 */
void GameManager::applyTickUpdates(GameScene* scene) {
  setActions(&scene->players[0], &scene->inputs[0]);
  setActions(&scene->players[1], &scene->inputs[1]);
  updateFrames(&scene->players[0], &scene->inputs[0]);
  updateFrames(&scene->players[1], &scene->inputs[1]);
  applyMovement(&scene->players[0]);
  applyMovement(&scene->players[1]);
}

/**
 * @brief Function to return a pointer to a player
 * @param pid Player 1 pid = 0 | Player 2 pid = 1
 */
PlayerEntity* GameManager::getPlayer(unsigned int pid) {
  return &allocator.getCurrentScene()->players[pid];
}
