/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "engine.hpp"

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

void printPlayerFrames(const PlayerEntity* p) {
  std::cout << "s: " << p->f_startup 
  << " a: " << p->f_active 
  << " r: " << p->f_recovery 
  << " tot: " << p->cur_attack->getCurAtkFrame(p->f_startup, p->f_active, p->f_recovery) 
  << std::endl;
}

/***************************
 ********* ATTACKS *********
 ***************************/

Attack::Attack(
    std::string name_,
    ButtonName button_, 
    std::vector<NumPadDir> motion_, 
    unsigned int f_window_,
    unsigned int f_startup_,
    unsigned int f_active_,
    unsigned int f_recovery_
  ) {
  name = name_;

  button = button_;
  motion = motion_;

  f_window = f_window_;
  f_startup = f_startup_;
  f_active = f_active_;
  f_recovery = f_recovery_;
}

unsigned int Attack::getTotalFrames() const {
  return f_active + f_recovery + f_startup;
}

const std::vector<BoxEntity>* Attack::getHitboxes(unsigned int f_s, unsigned int f_a, unsigned int f_r) const {
  unsigned int cur_f = getCurAtkFrame(f_s, f_a, f_r);
  // std::cout << "Recieved hitbox request\n";
  // std::cout << "Current attack frame is: " << cur_f << std::endl;
  // std::cout << "Size of hitbox sets for atk: " << hitbox_sets.size() << std::endl;
  return &hitbox_sets.at(cur_f);
}
const std::vector<BoxEntity>* Attack::getHurtboxes(unsigned int f_s, unsigned int f_a, unsigned int f_r) const {
  return &hurtbox_sets.at(getCurAtkFrame(f_s,f_a,f_r));
}

unsigned int Attack::getCurAtkFrame(unsigned int f_s, unsigned int f_a, unsigned int f_r) const {
  unsigned int idx = 0;
  if (f_s) {
    idx = f_startup - f_s;
  } else if (f_a) {
    idx = f_startup + (f_active - f_a);
  } else if (f_r) {
    idx = f_startup + f_active + (f_recovery - f_r);
  }
  return idx;
}

bool PlayerEntity::isAttacking() {
  return (state == GROUND_NORMAL) || (state == GROUND_SPECIAL) 
  || (state == AIR_NORMAL) || (state == AIR_SPECIAL);
}
/***************************
 **** PLAYER CONTROLLER ****
 ***************************/

PlayerController::PlayerController() {
  initializeCharacterAttrs();
  initializeAttacks();
}

void PlayerController::initializeAttacks() {}
void PlayerController::initializeCharacterAttrs() {}

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
    case (AIR_NORMAL): {return "AIR_NORMAL";}
    case (AIR_SPECIAL): {return "AIR_SPECIAL";}
    case (GROUND_NORMAL): {return "GND_NORMAL";}
    case (GROUND_SPECIAL): {return "GND_SPECIAL";}
  }
  return "NULL";
}

int PlayerController::getCharacterId() {
  std::cout << "Character ID request recieved for non-overloaded PC" << std::endl;
  return -1;
}

bool PlayerController::isActionable(PlayerEntity* p) {
  return (p->f_startup + p->f_active + p->f_recovery + p->f_hitstun == 0);
}

bool PlayerEntity::isGrounded() {
  return y_pos + height >= GAME_BORDER_Y1;
}

bool PlayerController::holdingForward(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->right) || (!p->facing_right && in->left);
}

bool PlayerController::holdingBack(const PlayerEntity* p, const ButtonStates* in) {
  return (p->facing_right && in->left) || (!p->facing_right && in->right);
}

bool PlayerController::checkMotionInputs(std::vector<NumPadDir> motion, unsigned int window, const NumPadDir* buf) {
  // No motion -> goes through
  if (motion.size() == 0) { return true; }

  // Iterating backwards through the given motion
  unsigned int j = motion.size()-1;
  // Negative is count of sequential mismatches
  // Positive is count of sequential matches
  // 0 is first instance of either
  int n_seq_matches = 0;

  // Current direction; sequential repeats are OK
  NumPadDir cur_dir = motion[j];

  // Iterating backwards in time (forwards in mem) through motion buffer
  for (unsigned int i = 0; i < window; ++i) {
    // Base case, end of motion reached
    if (j == -1) { return true; }

    // Current direction in motion buffer
    if (cur_dir == buf[i]) { 
      // If first match, decrement motion INDEX
      if (n_seq_matches <= 0) { j--; }

      // Update match count
      n_seq_matches = std::max(0, n_seq_matches + 1);

    } else {
      // Motion does not match buffer

      // Mismatches <= 0
      n_seq_matches = std::min(0, n_seq_matches-1);
      
      // Failure after reaching MAX_N_FUZZY_FRAMES mismatches
      if (n_seq_matches < -MAX_N_FUZZY_FRAMES) { return false; }

      // Progress the motion buffer on first mismatch
      if (n_seq_matches == 0) { cur_dir = motion[j]; }
    }

  }
  // Just in case
  return false;
}

bool PlayerController::checkAttacks(PlayerEntity* p, const ButtonStates* in, const std::vector<Attack>* attacks) {
  // All attacks have a corresponding button
  if (!isAnyButtonPressed(in)) { return false; }
  if (!attacks) { std::cerr << "There's no attacks lol\n";}
  // std::cout << attacks->size() << std::endl;
  // Check if any specials should begin
  for (unsigned int i = 0; i < attacks->size(); ++i) {
    const Attack* atk = &attacks->at(i);
    ButtonName b = atk->button;
    // std::cout << printButtonState(&in->b1) << std::endl;
    if (getButtonState(in, b) == PRESSED) {
      // std::cout << "button pressed\n";
      if (checkMotionInputs(atk->motion, atk->f_window, in->dir_buffer)) {
        // Button and motion both confirmed
        // Begin the first attack that is matched
        attack(p, in, atk);
        return true;
      }
    }
  }
  return false;
}

Coordinate PlayerController::getPlayerCenter(PlayerEntity* p) {
  Coordinate ret_coord;
  ret_coord.x = p->x_pos + p->width/2;
  ret_coord.y = p->y_pos + p->height/2;
  return ret_coord;
}

void PlayerController::updateBoxes(PlayerEntity* p) {
  const Attack* atk = p->cur_attack;
  if (atk == nullptr) {
    p->base_hurtboxes = &default_hurtboxes;
    p->base_hitboxes = &default_hitboxes;
    // std::cout << "No attack, default boxes:" << std::endl;
    // std::cout << "   hit: " << p->base_hitboxes->at(0) << std::endl;
    // std::cout << "   hurt: " << p->base_hurtboxes->at(0) << std::endl;
  } else {
    p->base_hitboxes = atk->getHitboxes(p->f_startup, p->f_active, p->f_recovery);
    p->base_hurtboxes = atk->getHurtboxes(p->f_startup, p->f_active, p->f_recovery);
    // std::cout << "Yes attack, base boxes:" << std::endl;
    // std::cout << "   hit: " << p->base_hitboxes->at(0) << std::endl;
    // std::cout << "   hurt: " << p->base_hurtboxes->at(0) << std::endl;
  }
  p->hitboxes.clear();
  p->hurtboxes.clear();
  // std::cout << "Should be 0: " << p->hitboxes.size() << std::endl;
  // std::cout << "Should be 0: " << p->hurtboxes.size() << std::endl;

  // Get the player's current center
  Coordinate p_cen = getPlayerCenter(p);

  // Loop through player's hitboxes
  for (size_t i = 0; i < p->base_hitboxes->size(); i++) {
    const BoxEntity* ref_box = &p->base_hitboxes->at(i);
    BoxEntity box;
    if (p->facing_right) {
      box.x = p_cen.x + ref_box->x;
    } else {
      box.x = (p_cen.x - ref_box->x) - ref_box->width;
    }
    box.y = p_cen.y + ref_box->y - p->height/2;
    box.width = ref_box->width;
    box.height = ref_box->height;
    // std::cout << "Ref hitbox: " << *ref_box << std::endl;
    // std::cout << "New hitbox: " << box << std::endl;
    p->hitboxes.push_back(box);
  }

  for (size_t i = 0; i < p->base_hurtboxes->size(); i++) {
    const BoxEntity* ref_box = &p->base_hurtboxes->at(i);
    BoxEntity box;
    if (p->facing_right) {
      box.x = p_cen.x + ref_box->x;
    } else {
      box.x = (p_cen.x - ref_box->x) - ref_box->width;
    }
    box.y = p_cen.y + ref_box->y - p->height/2;
    box.width = ref_box->width;
    box.height = ref_box->height;
    // std::cout << "Ref hurtbox: " << *ref_box << std::endl;
    // std::cout << "New hurtbox: " << box << std::endl;
    p->hurtboxes.push_back(box);
  }
  // std::cout << "Player has " << p->hitboxes.size() << " hitboxes." << std::endl;
  // std::cout << "Player has " << p->hurtboxes.size() << " hurtboxes." << std::endl;
}

/**
 * @brief Set a player's state given their current state and inputs
 */
void PlayerController::updateState(PlayerEntity* p, const ButtonStates* in) {
  if (isActionable(p)) {
    float prev_mod = p->v_mod;
    p->v_mod = p->facing_right ? 1.0 : -1.0; // Velocities set based on dir facing
    // End dash when you get crossed over
    if (p->state == DASH && prev_mod != p->v_mod) { 
      stand(p, in); 
    }
  }
  if (p->f_hitstun) {
    p->state = HITSTUN; 
    p->f_recovery = 0; p->f_startup = 0; p->f_active = 0;
  }
  if(!p->isAttacking()) { p->has_hit = false; }
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
    case (GROUND_NORMAL): { handleAttack(p, in); break; }
    case (GROUND_SPECIAL): { handleAttack(p, in); break; }
    case (AIR_NORMAL): { handleAttack(p, in); break; }
    case (AIR_SPECIAL): { handleAttack(p, in); break; }
    case (HITSTUN): { handleHitstun(p, in); break; }
    default: { 
      std::cerr << "State not caught: " << p->state << std::endl; 
      handleStand(p, in); break; 
    }
  }
}

/*
Functions that handle what to do in the state you're already in
*/
// Most grounded actions start here
void PlayerController::handleGrounded(PlayerEntity* p, const ButtonStates* in) {
  
  if (checkAttacks(p, in, &gnd_specials)) { return; }
  if (checkAttacks(p, in, &gnd_normals)) { return; }
  
  if (in->up) { 
    jump(p, in); 
  } else if (in->l2 == PRESSED) {
    // if (in->l2 != PRESSED) { return; }
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
  p->block = holdingBack(p, in) && isActionable(p);
}

void PlayerController::handleStand(PlayerEntity* p, const ButtonStates* in) {
  handleGrounded(p, in);
}
void PlayerController::handleWalkForwards(PlayerEntity* p, const ButtonStates* in) { 
  // if (holdingForward(p, in)) {
  //   p->x_vel = abs(p->x_vel) - walking_v < 0 ? p->x_vel : walking_v * p->v_mod;
  // }
  if (abs(p->x_vel) <= abs(walking_v)){
    p->x_vel = 0;
  }
  handleGrounded(p, in);
}
void PlayerController::handleWalkBackwards(PlayerEntity* p, const ButtonStates* in) { 
  // p->x_vel = abs(p->x_vel) - abs(backwalking_v) < 0 ? p->x_vel : backwalking_v;
  if (abs(p->x_vel) <= abs(backwalking_v)){
    p->x_vel = 0;
  }
  handleGrounded(p, in);
}
void PlayerController::handleDash(PlayerEntity* p, const ButtonStates* in) {
  if (holdingBack(p, in)) { 
    walkBackwards(p, in); 
    return;
  }

  if (in->up) { jump(p, in); return; }

  if (!in->l2 && !holdingForward(p, in)) { 
    stand(p, in); 
    p->x_vel = walking_v * p->v_mod;
    return; 
  }

  if (p->x_vel * p->v_mod >= 0) {
    p->x_vel = dash_v * p->v_mod;
  } else {
    p->x_vel += dash_acc * p->v_mod;
  }
}
void PlayerController::handleBackdash(PlayerEntity* p, const ButtonStates* in) {
  if (p->f_invuln > 0) { p->f_invuln--; }
  if (p->f_recovery < f_backdash_recovery / 2) {
    p->x_vel *= 0.8;
  }
  // Last frame of backdash?
  if (p->f_recovery <= 0) {
    p->x_vel = 0;
    stand(p, in);
    return;
  } else {
    p->f_recovery--;
  }
}

// Most aerial actions start here
bool PlayerController::handleAerial(PlayerEntity* p, const ButtonStates* in) {
  // Land or apply gravity
  if (p->isGrounded()) { 
    p->f_active = 0; p->f_startup = 0; p->f_recovery = 0;
    stand(p, in); 
  }
  else { p->y_vel += gravity; }

  applyAirStrafe(p, in);

  if (!isActionable(p)) { return false; }
  // Check for aerial attack inputs
  if (checkAttacks(p, in, &air_specials)) { return true; }
  if (checkAttacks(p, in, &air_normals)) { return true; }

  // Things requiring an air action
  if (p->air_action_cnt >= p->air_action_max) { return false; }

  if (in->l2 == PRESSED && holdingBack(p, in)) { airBackDash(p, in); return true; }
  if (in->l2 == PRESSED) { airDash(p, in); return true; }
  if (in->up == PRESSED) { jump(p, in); return true; }

  return false;
}

void PlayerController::handleFall(PlayerEntity* p, const ButtonStates* in) {
  // Basic Air Movement
  if (in->down) { p->y_vel += fastfall_v; }

  if (handleAerial(p, in)) { return; }
}
void PlayerController::handleJump(PlayerEntity* p, const ButtonStates* in) {
  // Things that can be done in recovery
  // Fall when falling
  if (p->y_vel >= 0) { fall(p, in); }

  if (p->f_recovery > 0) { p->f_recovery--; }

  // Things that cannot be done in recovery
  if (handleAerial(p, in)) { return; }
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
  if (checkAttacks(p, in, &gnd_specials)) { return; }
  if (checkAttacks(p, in, &gnd_normals)) { return; }
  p->x_vel = 0;
  if (!in->down) {
    stand(p, in);
  }
  p->block = holdingBack(p, in) && isActionable(p);
}

void PlayerController::handleAttack(PlayerEntity* p, const ButtonStates* in) {
  std::cout << "Error: Attack handled as blank character\n";
}

void PlayerController::handleHitstun(PlayerEntity* p, const ButtonStates* in) {
  p->y_vel += gravity * p->g_mult;

  if (--p->f_hitstun == 0) {
    if (p->isGrounded()) {
      stand(p, in);
    } else {
      fall(p, in);
    }
  }
}

/*
Functions to put you into a given state
*/
void PlayerController::stand(PlayerEntity* p, const ButtonStates* in) {
  p->y_vel = 0;
  // updateBoxes(p);
  // p->base_hitboxes = &default_hitboxes;
  // p->base_hurtboxes = &default_hurtboxes;
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
    // Moving slower than dash speed
    p->x_vel = dash_v * p->v_mod; // set speed to initial dash speed
  } 
  // else {
  //   p->x_vel += dash_acc * p->v_mod;
  // }
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
  std::vector<BoxEntity> crouching_box = default_hurtboxes;
  crouching_box[0].height = default_hurtboxes[0].height / 2;
  p->hurtboxes = crouching_box;
  p->x_vel = 0;
}

void PlayerController::attack(PlayerEntity* p, const ButtonStates* in, const Attack* atk) {
  p->state = atk->state;
  p->cur_attack = atk;
  p->f_startup = atk->f_startup;
}

bool PlayerEntity::preventStageCollisionFloor() {
  if (y_pos + y_vel + height > GAME_BORDER_Y1) {
    // p->y_vel = GAME_BORDER_Y1 - (p->y_pos + p->height);
    y_vel = 0;
    y_pos = GAME_BORDER_Y1 - height;
    return true;
  }
  return false;
}

bool PlayerEntity::preventStageCollisionLeft() {
  if (x_pos + x_vel < GAME_BORDER_X0) {
    x_vel = 0;
    x_pos = GAME_BORDER_X0;
    return true;
  }
  return false;
}

bool PlayerEntity::preventStageCollisionRight() {
  if (x_pos + x_vel + width > GAME_BORDER_X1) {
    x_vel = 0;
    x_pos = GAME_BORDER_X1 - width;
    return true;
  }
  return false;
}

/**
 * @brief Apply movement to a player using their velocities
 * @param p Pointer to the player's entity in the game scene
 * @note Handles collision
 */
void PlayerController::applyMovement(PlayerEntity* p) {
  // Don't collide left wall
  p->preventStageCollisionLeft();
  p->preventStageCollisionRight();
  p->preventStageCollisionFloor();

  // Apply velocities
  p->x_pos += p->x_vel;
  p->y_pos += p->y_vel;

  // Friction
  if (p->isGrounded()) {
    float fric_acc = p->x_vel / friction;
    // Check min speed
    if (abs(p->x_vel) < 2.5) {
      p->x_vel = 0;
    }
    // Apply friction
    if (p->x_vel != 0) {
      p->x_vel -= fric_acc; 
    }
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
  net_pindex = 1;
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
    return nullptr;
  }
  if (net_pindex > 1) {
    std::cerr << "Error: Rollback requested when allocator initialized with no defined"
    << " online player" << std::endl;
    return nullptr;
  }
  // Update current tick label (marking that we've rolled back)
  cur_tick = prev_tick;
  // Copy net inputs from param to scene's net pindex input
  GameScene *cur_scene = getCurrentScene();
  memcpy(&cur_scene->inputs[net_pindex], in, sizeof(ButtonStates));
  // Update the button states based on previous scene's inputs
  handleButtonStateTick(getInputsAtTick(net_pindex, prev_tick-1), &cur_scene->inputs[net_pindex]);
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

  // Update button states (held / press / released / just_released)
  handleButtonStateTick(&cur_scene->inputs[net_pindex], &next_scene->inputs[net_pindex]);
  handleButtonStateTick(&cur_scene->inputs[loc_pindex], &next_scene->inputs[loc_pindex]);

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

GameManager::GameManager(PlayerController* p1, PlayerController* p2, unsigned int net_pindex_) {
  cur_tick = INITIAL_FRAME;

  net_pindex = net_pindex_;
  if (net_pindex == 0) { loc_pindex = 1; }
  else if (net_pindex == 1) { loc_pindex = 0; }
  else { std::cerr << "Invalid net pindex\n"; }
  allocator = GameAllocator(net_pindex);

  allocator.loc_pindex = loc_pindex;
  allocator.net_pindex = net_pindex;

  players[0] = p1;
  players[1] = p2;

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
 * @brief Sends local inputs to the game engine at the current tick
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

  // Clean up local input states (held vs pressed, etc)
  handleButtonStateTick(allocator.getInputsAtTick(pindex, cur_tick-1), cur_inputs);
}

void GameManager::tick() {
  cur_tick++;
  GameScene* cur_scene = allocator.getNextScene();
  applyTickUpdates(cur_scene);
}

/**
 * @brief Roll the allocator back to a given frame, insert the recieved inputs,
 * then resimulate forwards until the current frame.
 * @note rollBack only changes the inputs of the remote player.
 */
void GameManager::rollBack(unsigned int frame, const ButtonStates* in) {
  // Move the current game scene back to when this input was sent
  // Send the input to the allocator
  GameScene *scene = allocator.rollBack(frame, in);
  
  // Ensure that roll *back* is *back*
  if (!scene) {return;}
  applyTickUpdates(scene);

  // Iterate through previous frames until the present
  for (unsigned int i = frame; i < cur_tick-1; ++i) {
    scene = allocator.rollForward();
    applyTickUpdates(scene);
  }
}

/**
 * @brief Private helper functions to apply all game logic to a scene
 * based on the previous scene's state.
 */
void GameManager::applyTickUpdates(GameScene* scene) {
  allocator.populateDirBuffer(cur_tick, 0);
  allocator.populateDirBuffer(cur_tick, 1);

  PlayerController* p1_con = players[0];
  PlayerController* p2_con = players[1];

  PlayerEntity* p1_ent = &scene->players[0];
  PlayerEntity* p2_ent = &scene->players[1];

  ButtonStates* p1_but = &scene->inputs[0];
  ButtonStates* p2_but = &scene->inputs[1];

  // Tick the player's state machines
  p1_con->updateState(p1_ent, p1_but);
  p2_con->updateState(p2_ent, p2_but);

  handlePlayerCollisions(p1_ent, p2_ent);

  // Apply movement based on new velocities
  p1_con->applyMovement(p1_ent);
  p2_con->applyMovement(p2_ent);

  // Update hitboxes to current positions
  p1_con->updateBoxes(p1_ent);
  p2_con->updateBoxes(p2_ent);

  handleAttackCollisions(p1_ent, p2_ent);
  handleAttackCollisions(p2_ent, p1_ent);

  // Update which direction the players are facing
  setFacingDir(p1_ent, p2_ent);
}

void GameManager::handleAttackCollisions(PlayerEntity* src, PlayerEntity* dst) {
  // Prevent multi-collision from same attack
  if (src->has_hit) { return; }

  bool hit = false;
  BoxEntity hitbox, hurtbox;
  const Attack *atk = src->cur_attack;

  // Loop through hitboxes
  for (size_t i = 0; i < src->hitboxes.size(); ++i) {
    hitbox = src->hitboxes.at(i); // set hitbox
    // Loop through hurtboxes
    for (size_t j = 0; j < dst->hurtboxes.size(); ++j) {
      hurtbox = dst->hurtboxes.at(j); // set hurtbox
      // Check collision
      if (hitbox.checkCollision(&hurtbox) || hurtbox.checkCollision(&hitbox)) {
        hit = true; break;
      }
    }
    // Collision found -> break
    if (hit) {break;}
  }
  // No collisions -> return
  if (!hit) { return; }

  if (dst->block) {
    std::cout << "Blocked" << std::endl;
    return;
  }

  // Update hit player's frames
  unsigned int src_frames_remaining = atk->getTotalFrames(); 
  src_frames_remaining -= atk->getCurAtkFrame(src->f_startup, src->f_active, src->f_recovery);

  // Notify counter-hit
  float counterhit = 1.0;
  if (dst->isAttacking()) { 
    counterhit = 2.0;
    std::cout << "COUNTER" << std::endl; 
    // dst-> = src_frames_remaining - atk->level * counterhit;
  }

  src->has_hit = true;
  std::cout << "!HIT LANDED!" << std::endl;

  // Apply damage
  dst->health -= atk->damage * dst->proration;
  // Apply hit vector
  dst->x_vel = atk->x_vel * src->v_mod;
  dst->y_vel = atk->y_vel * dst->g_mult;

  // Update proration
  dst->proration *= atk->proration;
  // Update gravity multiplier
  dst->g_mult += 0.1;

  dst->f_hitstun = src_frames_remaining + atk->level * counterhit;
}

void GameManager::handlePlayerCollisions(PlayerEntity* p1, PlayerEntity* p2) {
  // Boxes after applying velocity
  BoxEntity p1_box(p1->x_pos + p1->x_vel, p1->y_pos, p1->width, p1->height);
  BoxEntity p2_box(p2->x_pos + p2->x_vel, p2->y_pos, p2->width, p2->height);
  Coordinate p1_cen = p1_box.getCenter();
  Coordinate p2_cen = p2_box.getCenter();

  PlayerEntity *left_p, *right_p;
  Coordinate *left_cen, *right_cen;
  BoxEntity *left_box, *right_box;
  bool x_aligned = false;

  if (p1_box.checkCollision(&p2_box)) {
    // Determine which player on the left and which player on the right
    if (p1_cen.x <= p2_cen.x) {
      left_p = p1; left_box = &p1_box; left_cen = &p1_cen;
      right_p = p2; right_box = &p2_box; right_cen = &p2_cen;
    } else if (p1_cen.x > p2_cen.x) {
      left_p = p2; left_box = &p2_box; left_cen = &p2_cen;
      right_p = p1; right_box = &p1_box; right_cen = &p1_cen;
    } else {
      x_aligned = true; // currently impossible
    }
  } else {
    return; // No collision - return
  }

  float p_x_avg = (p1_cen.x + p2_cen.x)/2;
  float stage_center = (float) ((float) GAME_BORDER_X0 + (float) GAME_BORDER_X1) / 2; 

  // Edge case
  if (x_aligned) {
    // TODO: Implement this (one player centered on top of the other)
    return;
  }

  float overlap_w = right_p->x_pos - (left_p->x_pos + left_p->width);
  overlap_w = abs(overlap_w * (overlap_w < 0));
  
  float mean_vel = (right_p->x_vel + left_p->x_vel) / 10;

  // Left cornered
  if (left_p->preventStageCollisionLeft() || left_p->x_pos == GAME_BORDER_X0) {
    // Left player is pressed against corner
    if (right_p->isGrounded()) {
      // Right player is grounded, snap to left player
      right_p->x_pos = left_p->x_pos + left_p->width;
      right_p->x_vel = 0;
    } else if (left_p->isGrounded()) {
      // Right player in air, slide out
      right_p->x_vel = overlap_w / 8;
    } else {
      // Both players in air
      right_p->x_pos = left_p->x_pos + left_p->width;
      right_p->x_vel = 0;
    }
    return;
  }

  // Right cornered
  if (right_p->preventStageCollisionRight() || right_p->x_pos == GAME_BORDER_X1 - right_p->width) {
    // right player is pressed against corner
    if (left_p->isGrounded() && right_p->isGrounded()) {
      // both grounded, snap right against left
      left_p->x_pos = right_p->x_pos - left_p->width;
      left_p->x_vel = 0;
    } else if (right_p->isGrounded()) {
      // left player in air, slide out
      left_p->x_vel = -overlap_w / 8;
    } else {
      // Both players in air
      left_p->x_pos = right_p->x_pos - left_p->width;
      left_p->x_vel = 0;
    }
    return;
  }
  
  // Default midscreen collisions
  // Average their velocities and only apply avg if pushes them away more than not
  left_p->x_vel = std::min(left_p->x_vel, mean_vel);
  right_p->x_vel = std::max(right_p->x_vel, mean_vel);
  
  if (left_p->x_vel > 0) {
    left_p->x_pos = right_p->x_pos - left_p->width;
  }
  if (right_p->x_vel < 0) {
    right_p->x_pos = left_p->x_pos + left_p->width;
  }

  overlap_w = right_p->x_pos - (left_p->x_pos + left_p->width);
  overlap_w = abs(overlap_w * (overlap_w < 0));
  float l_cen_x = left_p->x_pos + left_p->width/2;
  float r_cen_x = right_p->x_pos + right_p->width/2;
  float cen_x = (l_cen_x + r_cen_x)/2;

  if (overlap_w > 0) {
    if (cen_x > stage_center) {
      // Clip left when players are right of center
      left_p->x_pos = right_p->x_pos - left_p->width;
    } else {
      right_p->x_pos = left_p->x_pos + left_p->width;
    }
  }
  // // Unclip aerial collisions
  // left_p->x_vel -= overlap_w / 8;
  // right_p->x_vel += overlap_w / 8;
  
}

/**
 * @brief Function to return a pointer to a player
 * @param pid Player 1 pid = 0 | Player 2 pid = 1
 */
PlayerEntity* GameManager::getPlayer(unsigned int pid) {
  return &allocator.getCurrentScene()->players[pid];
}

void GameManager::setFacingDir(PlayerEntity* p1, PlayerEntity* p2) {
  float p_dist = (p1->x_pos + p1->width/2) - (p2->x_pos + p2->width/2);
  if (p_dist < 0) {
    p1->facing_right = true;
    p2->facing_right = false;
  } else if (p_dist > 0) {
    p1->facing_right = false;
    p2->facing_right = true;
  }
}
