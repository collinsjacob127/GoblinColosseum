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
    if (!e->key.repeat) {
      if (e->key.scancode == bindings.up) { buttons.up = true; }
      else if (e->key.scancode == bindings.down) { buttons.down = true; }
      else if (e->key.scancode == bindings.left) { buttons.left = true; }
      else if (e->key.scancode == bindings.right) { buttons.right = true; }
      else if (e->key.scancode == bindings.b1 ) {buttons.b1 = true; }
      else if (e->key.scancode == bindings.b2 ) {buttons.b2 = true; }
      else if (e->key.scancode == bindings.b3) {buttons.b3 = true; }
      else if (e->key.scancode == bindings.b4) {buttons.b4 = true; }
      else if (e->key.scancode == bindings.l1) {buttons.l1 = true; }
      else if (e->key.scancode == bindings.r1) {buttons.r1 = true; }
      else if (e->key.scancode == bindings.l2) {buttons.l2 = true; }
      else if (e->key.scancode == bindings.r2) {buttons.r2 = true; }
    }
  } else if (e->type == SDL_EVENT_KEY_UP) {
    if (e->key.scancode == bindings.up) { buttons.up = false; }
    else if (e->key.scancode == bindings.down) { buttons.down = false; }
    else if (e->key.scancode == bindings.left) { buttons.left = false; }
    else if (e->key.scancode == bindings.right) { buttons.right = false; }
    else if (e->key.scancode == bindings.b1) {buttons.b1 = false; }
    else if (e->key.scancode == bindings.b2) {buttons.b2 = false; }
    else if (e->key.scancode == bindings.b3) {buttons.b3 = false; }
    else if (e->key.scancode == bindings.b4) {buttons.b4 = false; }
    else if (e->key.scancode == bindings.l1) {buttons.l1 = false; }
    else if (e->key.scancode == bindings.r1) {buttons.r1 = false; }
    else if (e->key.scancode == bindings.l2) {buttons.l2 = false; }
    else if (e->key.scancode == bindings.r2) {buttons.r2 = false; }
  }
}

void showButtonStates(const ButtonStates* btn) {
  std::cout << "Buttons: ";
  if (btn->up) {std::cout << "up ";}
  if (btn->down) {std::cout << "down ";}
  if (btn->left) {std::cout << "left ";}
  if (btn->right) {std::cout << "right ";}
  if (btn->b1) {std::cout << "b1 ";}
  if (btn->b2) {std::cout << "b2 ";}
  if (btn->b3) {std::cout << "b3 ";}
  if (btn->b4) {std::cout << "b4 ";}
  if (btn->l1) {std::cout << "l1 ";}
  if (btn->l2) {std::cout << "l2 ";}
  if (btn->r1) {std::cout << "r1 ";}
  if (btn->r2) {std::cout << "r2 ";}
  std::cout << std::endl;
}

void InputSystem::resetButtonStates() {
  buttons = ButtonStates();
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
  // Copy net inputs from param to scene's net pindex input
  memcpy(&getCurrentScene()->inputs[net_pindex], in, sizeof(ButtonStates));
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

/***************************
 **** PLAYER CONTROLLER ****
 ***************************/

PlayerController::PlayerController() { }

void PlayerController::testCharacterInclude() {
  std::cout << "Just the base player controller, nothing to see here..." << std::endl;
}

bool PlayerController::isActionable(PlayerEntity* p) {
  return (p->f_startup + p->f_active + p->f_recovery == 0);
}

bool PlayerController::isGrounded(PlayerEntity* p) {
  return p->y_pos + p->height >= GAME_BORDER_Y1;
}

/**
 * @brief Set a player's state given their current state and inputs
 */
void PlayerController::updateState(PlayerEntity* p, const ButtonStates* in) {
  if (!isActionable(p)) { return; }

  // Grounded movement
  if (isGrounded(p)) {
    // Reset # air actions once grounded
    p->air_action_cnt = 0;
    // Jump
    if (in->up) {
      p->y_vel = jumping_v;
      p->f_recovery = f_jumping_recovery;
    }
    // Walk left
    if (in->left && p->x_vel <= 0) {
      p->x_vel = -walking_v;
    }
    // Walk right if stopped
    if (in->right && p->x_vel >= 0) {
      p->x_vel = walking_v;
    }
    // Facing forward, sprint
    if (p->facing_right && in->right && in->l2) {
      p->x_vel = dash_v;
    } else if (!p->facing_right && in->left && in->l2) {
      p->x_vel = -dash_v;
    }
    // Holding back, backdash
    if (p->facing_right && in->left && in->l2) {
      p->x_vel = backdash_v;
      p->f_recovery = f_backdash_recovery;
    } else if (!p->facing_right && in->right && in->l2) {
      p->x_vel = -backdash_v;
      p->f_recovery = f_backdash_recovery;
    }
 // Aerial Movement
 } else {
    // Fast fall if in the air
    if (in->down) {
      p->y_vel += fastfall_v;
    }

    // ALL BELOW USE / REQUIRE AN AIR ACTION
    if (p->air_action_cnt < p->air_action_max) {

    if (in->up) {
      p->y_vel = jumping_v;
      p->air_action_cnt++;
      p->f_recovery = f_jumping_recovery;
    }

    // Holding forward, airdash
    if (holdingForward(p, in) && in->l2) {
      if (p->facing_right) {
        p->x_vel = airdash_v;
      } else {
        p->x_vel = -airdash_v;
      }
      p->f_recovery = f_airdash_recovery;
      p->air_action_cnt++;
    }

    // Holding back, backdash
    if (holdingBack(p, in) && in->l2) {
      if (p->facing_right) {
        p->x_vel = backdash_v;
      } else {
        p->x_vel = -backdash_v;
      }
      p->f_recovery = f_backdash_recovery;
      p->air_action_cnt++;
    }
    }
 }
 switch (p->state) {
  case (STAND): { handleStand(p, in); break; }
  case (CROUCH): { handleCrouch(p, in); break; }
  case (FALL): { handleFall(p, in); break; }
  case (WALKF): { handleWalkForwards(p, in); break; }
  case (WALKB): { handleWalkBackwards(p, in); break; }
  case (DASH): { handleDash(p, in); break; }
  case (BACKDASH): { handleBackdash(p, in); break; }
  case (JUMP): { handleJump(p, in); break; }
  case (AIR_DASH): { handleAirDash(p, in); break; }
  case (AIR_BACKDASH): { handleAirBackDash(p, in); break; }
  case (ATTACK): { handleAttack(p, in); break; }
 }
}

// Most grounded actions start here
void PlayerController::handleStand(PlayerEntity* p, const ButtonStates* in) {
  if (in->l2) {
    if (holdingBack(p, in)) {
      backdash(p, in);
    } else {
      dash(p, in);
    }
  } else if (in->up) { 
    jump(p, in); 
  } else if (in->down) {
    crouch(p, in);
  } else if (holdingForward(p, in)) { 
    walkForwards(p, in); 
  } else if (holdingBack(p, in)) {
    walkBackwards(p, in);
  }
}
void PlayerController::handleWalkForwards(PlayerEntity* p, const ButtonStates* in) { handleStand(p, in); }
void PlayerController::handleWalkBackwards(PlayerEntity* p, const ButtonStates* in) { handleStand(p, in); }
void PlayerController::handleDash(PlayerEntity* p, const ButtonStates* in) { handleStand(p, in); }

// Most aerial actions start here
void PlayerController::handleFall(PlayerEntity* p, const ButtonStates* in) {

}

void PlayerController::handleBackdash(PlayerEntity* p, const ButtonStates* in) {

}
void PlayerController::handleJump(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::handleAirDash(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::handleAirBackDash(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::handleCrouch(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::handleAttack(PlayerEntity* p, const ButtonStates* in) { }

void PlayerController::stand(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::walkForwards(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::walkBackwards(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::fall(PlayerEntity* p, const ButtonStates* in) { }

void PlayerController::dash(PlayerEntity* p, const ButtonStates* in) {
  float v_mod = p->facing_right ? 1.0 : -1.0;
  // Check if first frame of dash
  if (abs(p->x_vel) < abs(dash_v)) {
    p->x_vel = dash_v * v_mod;
  } else {
    p->x_vel += dash_acc * v_mod;
  }
}
void PlayerController::backdash(PlayerEntity* p, const ButtonStates* in) { }

void PlayerController::jump(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::airDash(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::airBackDash(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::crouch(PlayerEntity* p, const ButtonStates* in) { }
void PlayerController::attack(PlayerEntity* p, const ButtonStates* in) { }

bool PlayerController::holdingForward(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->right) || (!p->facing_right && in->left);
}

bool PlayerController::holdingBack(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->left) || (!p->facing_right && in->right);
}


/**
 * @brief Apply movement to a player using their velocities
 * @param p Pointer to the player's entity in the game scene
 * @note Handles collision
 */
void PlayerController::applyMovement(PlayerEntity* p) {
  // Don't collide left wall
  if (p->x_pos + p->x_vel < GAME_BORDER_X0) {
    p->x_vel = 0;
    p->x_pos = GAME_BORDER_X0;
  }

  // Don't collide right wall
  if (p->x_pos + p->x_vel + p->width > GAME_BORDER_X1) {
    p->x_vel = 0;
    p->x_pos = GAME_BORDER_X1 - p->width;
  }

  // Don't collide floor
  if (p->y_pos + p->y_vel + p->height > GAME_BORDER_Y1) {
    p->y_vel = GAME_BORDER_Y1 - (p->y_pos + p->height);
  }

  // Apply velocities
  p->x_pos += p->x_vel;
  p->y_pos += p->y_vel;

  // Gravity
  if (!isGrounded(p)) {
    p->y_vel += gravity;
  }

  // Friction
  float v_mod = p->facing_right ? 1.0 : -1.0;
  if (isGrounded(p)) { p->x_vel -= (p->x_vel / friction) * v_mod; }
  if (abs(p->x_vel) < abs(friction)) {
    p->x_vel = 0;
  }
}

/*****************************
 ******** GAME MANAGER *******
 *****************************/
GameManager::GameManager() {
  cur_tick = 0;
  loc_pindex = 0;
  net_pindex = 1;
  allocator = GameAllocator(net_pindex);
  // TEMP - for testing player controller
  players[0] = new PlayerController();
  players[1] = new PlayerController();
  // Set starting positions
  GameScene* scene = allocator.getCurrentScene();
  PlayerEntity* p1 = &scene->players[0];
  PlayerEntity* p2 = &scene->players[1];
  p1->x_pos = 1400 - p1->width;
  p2->x_pos = 1800;
}

GameManager::GameManager(unsigned int net_p1_or_p2) {
  cur_tick = 0;
  net_pindex = net_p1_or_p2;
  if (net_pindex == 0) { loc_pindex = 1; }
  else if (net_pindex == 1) { loc_pindex = 0; }
  else { std::cerr << "Invalid net pindex\n"; }

  allocator.loc_pindex = loc_pindex;
  allocator.net_pindex = net_pindex;
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
  // Ensure that roll *back* is *back*
  if (frame >= cur_tick) {return;}
  GameScene* scene = allocator.rollForward();
  // Iterate through previous frames until the present
  for (unsigned int i = frame; i < cur_tick-1; ++i) {
    applyTickUpdates(scene);
    scene = allocator.rollForward();
  }
}

/**
 * @brief Private helper functions to apply all game logic to a scene
 * based on the previous scene's state.
 */
void GameManager::applyTickUpdates(GameScene* scene) {
  players[0]->updateState(&scene->players[0], &scene->inputs[0]);
  players[1]->updateState(&scene->players[1], &scene->inputs[1]);
  players[0]->updateFrames(&scene->players[0], &scene->inputs[0]);
  players[1]->updateFrames(&scene->players[1], &scene->inputs[1]);
  players[0]->applyMovement(&scene->players[0]);
  players[1]->applyMovement(&scene->players[1]);
  setFacingDir(&scene->players[0], &scene->players[1]);
}

/**
 * @brief Function to return a pointer to a player
 * @param pid Player 1 pid = 0 | Player 2 pid = 1
 */
PlayerEntity* GameManager::getPlayer(unsigned int pid) {
  return &allocator.getCurrentScene()->players[pid];
}

void GameManager::setFacingDir(PlayerEntity* p1, PlayerEntity* p2) {
  if (p1->x_pos + p1->width/2 < p2->x_pos + p2->width/2) {
    p1->facing_right = true;
    p2->facing_right = false;
  } else {
    p1->facing_right = false;
    p2->facing_right = true;
  }
}
