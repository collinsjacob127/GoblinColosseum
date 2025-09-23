/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

/*****************************
 ******** INPUT SYSTEM *******
 *****************************/
InputSystem::InputSystem() {
  for (unsigned int i = 0; i < MAX_INPUT_FRAMES; ++i) {
    buttons.dir_buffer[i] = CENTER;
  }
}

void applyButtonUpdate(const Button* prev_btn, Button* cur_btn) {
  if (*cur_btn == 0) { return; }
  if (*prev_btn > 0) { *cur_btn = HELD; }
  return;
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
  if (btn_state->up) {std::cout << "up_" << printButtonState(&btn_state->up);}
  if (btn_state->down) {std::cout << "down_" << printButtonState(&btn_state->down);}
  if (btn_state->left) {std::cout << "left_" << printButtonState(&btn_state->left);}
  if (btn_state->right) {std::cout << "right_" << printButtonState(&btn_state->right);}
  if (btn_state->b1) {std::cout << "b1_" << printButtonState(&btn_state->b1);}
  if (btn_state->b2) {std::cout << "b2_" << printButtonState(&btn_state->b2);}
  if (btn_state->b3) {std::cout << "b3_" << printButtonState(&btn_state->b3);}
  if (btn_state->b4) {std::cout << "b4_" << printButtonState(&btn_state->b4);}
  if (btn_state->l1) {std::cout << "l1_" << printButtonState(&btn_state->l1);}
  if (btn_state->l2) {std::cout << "l2_" << printButtonState(&btn_state->l2);}
  if (btn_state->r1) {std::cout << "r1_" << printButtonState(&btn_state->r1);}
  if (btn_state->r2) {std::cout << "r2_" << printButtonState(&btn_state->r2);}
  std::cout << std::endl;
}

void getDirFromButtonState(const PlayerEntity* p, const ButtonStates* btn_state, NumPadDir* dir) {
  // clean-slate button for switching left & right
  ButtonStates btn;
  // Copy values from btn_state to our clean button
  btn.up = btn_state->up;
  btn.down = btn_state->down;
  btn.left = btn_state->left;
  btn.right = btn_state->right;
  // If facing left, switch left & right
  if (!p->facing_right) {
    Button tmp = btn.left;
    btn.left = btn.right;
    btn.right = tmp;
  }
  // Convert button states to numpad direction
  if (btn.up) {
    if (btn.left) { *dir = UP_LEFT; } 
    else if (btn.right) { *dir = UP_RIGHT; } 
    else { *dir = UP; }
  } else if (btn.down) {
    if (btn.left) { *dir = DOWN_LEFT; } 
    else if (btn.right) { *dir = DOWN_RIGHT; } 
    else { *dir = DOWN; }
  } else {
    if (btn.left) { *dir = LEFT; } 
    else if (btn.right) { *dir = RIGHT; } 
    else { *dir = CENTER; }
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

/***************************
 **** PLAYER CONTROLLER ****
 ***************************/

PlayerController::PlayerController() { }

void PlayerController::testCharacterInclude() {
  std::cout << "Just the base player controller, nothing to see here..." << std::endl;
}

std::string PlayerController::getStateString(const PlayerEntity* p) {
  switch (p->state) {
    case (STAND): {return "STANDING";}
    case (CROUCH): {return "CROUCHING";}
    case (JUMP): {return "JUMPING";}
    case (FALL): {return "FALLING";}
    case (WALKF): {return "WALKING FORWARDS";}
    case (WALKB): {return "WALKING BACKWARDS";}
    case (DASH): {return "DASHING";}
    case (BACKDASH): {return "BACKDASHING";}
    case (AIR_DASH): {return "AIR DASHING";}
    case (AIR_BACKDASH): {return "AIR BACKDASHING";}
    case (ATTACK): {return "ATTACKING";}
  }
  return "NULL";
}

bool PlayerController::isActionable(PlayerEntity* p) {
  return (p->f_startup + p->f_active + p->f_recovery == 0);
}

bool PlayerController::isGrounded(PlayerEntity* p) {
  return p->y_pos + p->height >= GAME_BORDER_Y1;
}

bool PlayerController::holdingForward(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->right) || (!p->facing_right && in->left);
}

bool PlayerController::holdingBack(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->left) || (!p->facing_right && in->right);
}


/**
 * @brief Set a player's state given their current state and inputs
 */
void PlayerController::updateState(PlayerEntity* p, const ButtonStates* in) {
  if (isActionable(p)) {
    p->v_mod = p->facing_right ? 1.0 : -1.0;
  }
  switch (p->state) {
    case (STAND): { handleStand(p, in); break; }
    case (CROUCH): { handleCrouch(p, in); break; }
    case (JUMP): { handleJump(p, in); break; }
    case (FALL): { handleFall(p, in); break; }
    case (WALKF): { handleWalkForwards(p, in); break; }
    case (WALKB): { handleWalkBackwards(p, in); break; }
    case (DASH): { handleDash(p, in); break; }
    case (BACKDASH): { handleBackdash(p, in); break; }
    case (AIR_DASH): { handleAirDash(p, in); break; }
    case (AIR_BACKDASH): { handleAirBackDash(p, in); break; }
    case (ATTACK): { handleAttack(p, in); break; }
  }
}

/*
Functions that handle what to do in the state you're already in
*/
// Most grounded actions start here
void PlayerController::handleStand(PlayerEntity* p, const ButtonStates* in) {
  if (in->up == PRESSED) { 
    jump(p, in); 
  } else if (in->l2 == PRESSED ) {
    if (holdingBack(p, in)) {
      backdash(p, in);
    } else {
      dash(p, in);
    }
  } else if (in->down) {
    crouch(p, in);
  } else if (holdingForward(p, in)) { 
    walkForwards(p, in); 
  } else if (holdingBack(p, in)) {
    walkBackwards(p, in);
  } else {
    stand(p, in);
  }
}
void PlayerController::handleWalkForwards(PlayerEntity* p, const ButtonStates* in) { 
  p->x_vel = 0;
  handleStand(p, in); 
}
void PlayerController::handleWalkBackwards(PlayerEntity* p, const ButtonStates* in) { 
  p->x_vel = 0;
  handleStand(p, in); 
}
void PlayerController::handleDash(PlayerEntity* p, const ButtonStates* in) {
  if (holdingBack(p, in)) { handleStand(p, in); return;}
  if (in->l2) {
    p->x_vel = dash_v * p->v_mod;
  } else {
    handleStand(p, in);
  }
}
void PlayerController::handleBackdash(PlayerEntity* p, const ButtonStates* in) {
  if (p->f_invuln > 0) { p->f_invuln--; }
  if (p->f_recovery < p->f_recovery / 2) {
    p->x_vel /= 2;
  }
  // Last frame of backdash?
  if (p->f_recovery <= 0) {
    stand(p, in);
    return;
  } else {
    p->f_recovery--;
  }
}

// Most aerial actions start here
void PlayerController::handleFall(PlayerEntity* p, const ButtonStates* in) {
  // Land or apply gravity
  if (isGrounded(p)) { stand(p, in); }
  else { p->y_vel += gravity; }

  // Basic Air Movement
  if (in->down) { p->y_vel += fastfall_v; }
  applyAirStrafe(p, in);

  // Things requiring an air action
  if (p->air_action_cnt >= p->air_action_max) { return; }

  if (in->l2 == PRESSED && holdingBack(p, in)) { airBackDash(p, in); return; }
  if (in->l2 == PRESSED) { airDash(p, in); return; }
  if (in->up == PRESSED) { jump(p, in); return; }
}
void PlayerController::handleJump(PlayerEntity* p, const ButtonStates* in) {
  // Things that can be done in recovery
  // Fall when falling
  if (p->y_vel >= 0) { fall(p, in); }

  // Gravity
  if (!isGrounded(p)) { p->y_vel += gravity; }

  // Basic Air Movement
  applyAirStrafe(p, in);

  // Things that cannot be done in recovery
  if (p->f_recovery > 0) { 
    p->f_recovery--;
    return; 
  }

  // Things requiring an air action
  if (p->air_action_cnt >= p->air_action_max) { return; }

  if (in->l2 == PRESSED && holdingBack(p, in)) { airBackDash(p, in); return; }
  if (in->l2 == PRESSED) { airDash(p, in); return; }
  if (in->up == PRESSED) { jump(p, in); return; }
}
void PlayerController::handleAirDash(PlayerEntity* p, const ButtonStates* in) {
  // Decrement recovery
  p->f_recovery--;

  // Fall after airdash
  if (p->f_recovery <= 0) {
    fall(p, in);
  }
}
void PlayerController::handleAirBackDash(PlayerEntity* p, const ButtonStates* in) {
  if (p->f_invuln > 0) { p->f_invuln--; }
  // Slow down after f0 of air backdash
  float acc = air_backdash_v / 4; // Target velocity at airdash end
  acc /= f_air_backdash_recovery;
  p->x_vel -= acc;
  // Decrement recovery
  p->f_recovery--;
  if (p->f_recovery <= 0) {
    fall(p, in);
  }
}
void PlayerController::handleCrouch(PlayerEntity* p, const ButtonStates* in) {
  p->x_vel = 0;
  handleStand(p, in);
}
void PlayerController::handleAttack(PlayerEntity* p, const ButtonStates* in) { 

}

/*
Functions to put you into a given state
*/
void PlayerController::stand(PlayerEntity* p, const ButtonStates* in) {
  p->state = STAND;
  p->air_action_cnt = 0;
}
void PlayerController::walkForwards(PlayerEntity* p, const ButtonStates* in) {
  p->state = WALKF;
  p->x_vel = walking_v * p->v_mod;
}
void PlayerController::walkBackwards(PlayerEntity* p, const ButtonStates* in) {
  p->state = WALKB;
  p->x_vel = backwalking_v * p->v_mod;
}
void PlayerController::jump(PlayerEntity* p, const ButtonStates* in) {
  p->state = JUMP;
  p->y_vel = jumping_v;
  p->f_recovery = f_jumping_recovery;
  p->air_action_cnt++;
}
void PlayerController::fall(PlayerEntity* p, const ButtonStates* in) {
  p->state = FALL;
}
void PlayerController::dash(PlayerEntity* p, const ButtonStates* in) {
  p->state = DASH;
  if (abs(p->x_vel) < abs(dash_v)) {
    p->x_vel = dash_v * p->v_mod;
  } else {
    p->x_vel += dash_acc * p->v_mod;
  }
}
void PlayerController::backdash(PlayerEntity* p, const ButtonStates* in) {
  p->state = BACKDASH;
  p->f_recovery = f_backdash_recovery;
  p->f_invuln = f_backdash_invuln;
  p->x_vel = backdash_v * p->v_mod;
}
void PlayerController::airDash(PlayerEntity* p, const ButtonStates* in) {
  p->state = AIR_DASH;
  p->x_vel = airdash_v * p->v_mod;
  p->y_vel = 0;
  p->f_recovery = f_airdash_recovery;
  p->air_action_cnt++;
}
void PlayerController::airBackDash(PlayerEntity* p, const ButtonStates* in) {
  p->state = AIR_BACKDASH;
  p->x_vel = air_backdash_v * p->v_mod;
  p->y_vel = 0;
  p->f_recovery = f_air_backdash_recovery;
  p->air_action_cnt++;
}
void PlayerController::crouch(PlayerEntity* p, const ButtonStates* in) {
  p->state = CROUCH;
  if (holdingBack(p, in)) {
    p->block = true;
  }
  p->x_vel = 0;
}
void PlayerController::attack(PlayerEntity* p, const ButtonStates* in) {
  p->state = ATTACK;
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

  // Friction
  if (isGrounded(p)) {
    float fric_acc = p->x_vel / friction;
    // Check min speed
    if (abs(p->x_vel) < 2.5) {
      p->x_vel = 0;
    }
    // Apply friction
    p->x_vel -= fric_acc; 
  }
}

void PlayerController::adjustVel(float* v_cur, float v_start, float v_final, float frac) {
  float acc = (v_final - v_start) * frac;
  if (abs(*v_cur + acc) > abs(v_final)) {
    *v_cur = v_final;
  } else {
    *v_cur += acc;
  }
}

void PlayerController::applyAirStrafe(PlayerEntity* p, const ButtonStates* in) {
  if (in->left) {
    if (p->x_vel < -airstrafe_v) { 
      // Can't strafe to go faster than strafe speed
      return; 
    }
    else if (p->x_vel <= airstrafe_v) { 
      // Within |airstrafe_v|, complete strafe control
      p->x_vel = -airstrafe_v;
    }
  }
  if (in->right) {
    if (p->x_vel > airstrafe_v) { 
      // Can't strafe to go faster than strafe speed
      return; 
    }
    else if (p->x_vel >= -airstrafe_v) { 
      // Within |airstrafe_v|, complete strafe control
      p->x_vel = airstrafe_v;
    }
  }
}

/*****************************
 ******* GAME ALLOCATOR ******
 *****************************/
GameAllocator::GameAllocator() {
  cur_tick = INITIAL_FRAME;
  net_pindex = 99;
  loc_pindex = 0;
  if (ENABLE_HELPER_PRINTOUTS) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Frame buffer allocated: " << (float) sizeof(history_buffer)/1024 << " Kb\n";
  }
}

GameAllocator::GameAllocator(unsigned int net_p1_or_p2) {
  cur_tick = INITIAL_FRAME;
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

unsigned int GameAllocator::getIndexOfTick(unsigned int tick) {
  return tick % HISTORY_BUFFER_SIZE;
}

unsigned int GameAllocator::getCurrentIndex() {
  return getIndexOfTick(cur_tick);
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

GameScene* GameAllocator::rollBack(unsigned int prev_tick, const ButtonStates* in) {
  if (cur_tick - prev_tick > HISTORY_BUFFER_SIZE || prev_tick < INITIAL_FRAME) {
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

GameScene* GameAllocator::getSceneAtTick(unsigned int tick) {
  return &history_buffer[getIndexOfTick(tick)];
}

const ButtonStates* GameAllocator::getInputsAtTick(int pindex, unsigned int tick) {
  if (pindex != 0 && pindex != 1) { std::cerr << "Allocator recieved bad index in getPrevInputs\n"; }

  const GameScene* scene = getSceneAtTick(tick);

  return &scene->inputs[pindex];
}

void GameAllocator::populateDirBuffer(unsigned int tick, int pindex) {
  GameScene* cur_scene = getSceneAtTick(tick);
  NumPadDir* dst_dir;

  // Start iteration at current tick
  unsigned int idx = tick;

  // Variable to hold the calculated NumPadDir
  NumPadDir temp = CENTER;
  GameScene* scene = getSceneAtTick(tick);

  // Loop through dir buffer 
  for (unsigned int i = 0; i < MAX_INPUT_FRAMES; ++i) {
    // Get pointer to the dir buffer at this tick
    dst_dir = &cur_scene->inputs[pindex].dir_buffer[i];
    // Calculate the correct index
    idx = cur_tick - i;
    // Get a pointer to the scene at this tick
    scene = getSceneAtTick(idx); 
    // Calculate the corresponding NumPadDir
    getDirFromButtonState(&scene->players[pindex], &scene->inputs[pindex], dst_dir);
  }
}


/*****************************
 ******** GAME MANAGER *******
 *****************************/
GameManager::GameManager() {
  cur_tick = INITIAL_FRAME;
  loc_pindex = 0;
  net_pindex = 1;
  allocator = GameAllocator(net_pindex);
  // TEMP - for testing player controller
  players[0] = new PlayerController();
  players[1] = new PlayerController();
  // Set starting positions
  setInitialPlayerPositions();
}

GameManager::GameManager(unsigned int net_p1_or_p2) {
  cur_tick = INITIAL_FRAME;

  net_pindex = net_p1_or_p2;
  if (net_pindex == 0) { loc_pindex = 1; }
  else if (net_pindex == 1) { loc_pindex = 0; }
  else { std::cerr << "Invalid net pindex\n"; }
  allocator.loc_pindex = loc_pindex;
  allocator.net_pindex = net_pindex;

  players[0] = new PlayerController();
  players[1] = new PlayerController();
  setInitialPlayerPositions();
}

void GameManager::setInitialPlayerPositions() {
  GameScene* scene = allocator.getCurrentScene();
  PlayerEntity* p1 = &scene->players[0];
  PlayerEntity* p2 = &scene->players[1];
  p1->x_pos = 1400 - p1->width;
  p2->x_pos = 1800;
  p1->y_pos = GAME_BORDER_Y1 - p1->height;
  p2->y_pos = GAME_BORDER_Y1 - p2->height;
}

/**
 * @brief Sends inputs from SDL_Event to InputSystem
 */
void GameManager::updateInputs(const ButtonStates* btns, int pindex) {
  // Player index must be 0 or 1
  if (pindex != 0 && pindex != 1) { std::cerr << "Invalid pindex: " << pindex << std::endl; }

  // Pointer to current game scene
  GameScene* cur_scene = allocator.getCurrentScene();
  if (!cur_scene) { std::cerr << "Game allocator returned null scene to input update request\n"; }

  // Pointer to button inputs of current scene
  ButtonStates* cur_inputs = &cur_scene->inputs[pindex];

  // Copy the passed inputs to the current scene
  memcpy(cur_inputs, 
         btns, 
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
  allocator.populateDirBuffer(cur_tick, 0);
  allocator.populateDirBuffer(cur_tick, 1);
  // Tick the player's state machines
  players[0]->updateState(&scene->players[0], &scene->inputs[0]);
  players[1]->updateState(&scene->players[1], &scene->inputs[1]);
  // Apply movement based on new velocities
  players[0]->applyMovement(&scene->players[0]);
  players[1]->applyMovement(&scene->players[1]);
  // Update which direction the players are facing
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
