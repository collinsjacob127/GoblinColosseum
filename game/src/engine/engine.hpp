/**
 * Author: Jacob Collins
 * Description:
 * This file contains headers for:
 * 
 * - Input System
 * The input system is an abstraction layer between system input and input commands
 * Based on input system designed by rcmagic (wrote GGST netcode)
 * https://github.com/rcmagic/DemoFighterWithNetcode/blob/master/game/InputSystem.lua
 * 
 * - Game Allocator
 * Allocates game state memory buffer, enabling rollback
 * Essentially an interface for the list of game scenes
 * 
 * - GameScene
 * Tracks entities and all information that is used for a given scene
 * 
 * - PlayerEntity
 * Struct containing information regarding the player's dynamic state
 * 
 * - Game Manager
 * Interface between main() game loop and the game engine
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <string.h>
#include <vector>

#include <SDL3/SDL.h>
#include "util.hpp"
#include "buttons.hpp"

#define ENABLE_BOX_DEBUG true

#define ENABLE_HELPER_PRINTOUTS true
#define ENABLE_MOTIONSCANNING_DEBUG true

// For rollback functionality demo:
#define MAX_ROLLBACK_FRAMES 60
#define FRAME_ADVANTAGE_LIMIT 5
#define INITIAL_FRAME 60
constexpr uint16_t MAX_GAME_DURATION = 60*60*5;

#define HISTORY_BUFFER_SIZE 120


#define MAX_N_HITBOXES 4
#define MAX_N_HURTBOXES 4

#define DEFAULT_XDIM 3200
#define DEFAULT_YDIM 1800

#define GAME_BORDER_X0 180
#define GAME_BORDER_X1 3020
#define GAME_BORDER_Y0 0
#define GAME_BORDER_Y1 1678

#define MAX_N_FUZZY_FRAMES 3

// Enum for states shared by all characters
enum State {
  // Movement
  STAND,
  CROUCH,
  FALL,
  WALKF,
  WALKB,
  DASH,
  BACKDASH,
  JUMP,
  AIR_DASH,
  AIR_BACKDASH,
  GROUND_NORMAL,
  GROUND_SPECIAL,
  AIR_NORMAL,
  AIR_SPECIAL,
  BLOCKSTUN,
  HITSTUN,
  SOFT_KNOCKDOWN,
  HARD_KNOCKDOWN
};

// Template class to list character attacks
class Attack {
 public:
  Attack(
    std::string name_,
    ButtonName button_, 
    std::vector<NumPadDir> motion_, 
    unsigned int f_window_,
    unsigned int f_startup_,
    unsigned int f_active_,
    unsigned int f_recovery_
  );

  int id = 0;
  float x_vel = 0;
  float y_vel = 0;
  float damage = 0;
  float proration = 1.0;
  int level = 1.0;

  // i.e., AIR_NORMAL, GROUND_NORMAL, AIR_SPECIAL, or GROUND_SPECIAL
  State state;
  std::string name;

  // Define button used to launch attack
  ButtonName button;
  std::vector<NumPadDir> motion; 

  // How many frames back should this check
  unsigned int f_window;

  // Hitboxes and hurtboxes associated with this attack
  const std::vector<BoxEntity>* getHitboxes(unsigned int f_s, unsigned int f_a, unsigned int f_r) const;
  std::vector<std::vector<BoxEntity>> hitbox_sets;

  const std::vector<BoxEntity>* getHurtboxes(unsigned int f_s, unsigned int f_a, unsigned int f_r) const;
  std::vector<std::vector<BoxEntity>> hurtbox_sets;

  unsigned int getCurAtkFrame(unsigned int f_s, unsigned int f_a, unsigned int f_r) const;
  unsigned int getTotalFrames() const;
  unsigned int f_startup;
  unsigned int f_active;
  unsigned int f_recovery;
};


/**
 * Struct for dynamic character info
 */
struct PlayerEntity {
  State state = STAND;
  const Attack* cur_attack;

  float v_mod = 1.0;

  bool block = false;

  float health = 1000;
  float proration = 1.0;
  float g_mult = 1.0;

  float x_pos = 1920.0/2.0;
  float y_pos = 1080.0/2.0;

  bool facing_right = true;
  bool has_hit = false;

  float x_vel = 0.0;
  float y_vel = 0.0;

  float width = 150.0;
  float height = 400.0;

  int air_action_cnt = 0;
  int air_action_max = 2;

  int f_startup = 0;
  int f_active = 0;
  int f_recovery = 0;

  int f_invuln = 0;
  int f_hitstun = 0;

  bool preventStageCollisionFloor();
  bool preventStageCollisionLeft();
  bool preventStageCollisionRight();
  bool isGrounded();
  bool isAttacking();

  const std::vector<BoxEntity>* base_hitboxes;
  const std::vector<BoxEntity>* base_hurtboxes;
  std::vector<BoxEntity> hitboxes;
  std::vector<BoxEntity> hurtboxes;
};

void printPlayerFrames(const PlayerEntity* p);

/**
 * @brief Function to set the numpad direction of a given button state and player.
 * @param p The player at a given tick
 * @param btn_state The button state at the same tick
 * @param dir The resulting direction will be saved here.
 * @note Numpad directions are given as though a player is facing right.
 * 7 8 9
 * 4 5 6
 * 1 2 3
 * 4 is left, 6 is right, 8 is up, 2 is down, 5 is no direction, etc.
 */
void getDirFromButtonState(const PlayerEntity* p, const ButtonStates* btn_state, NumPadDir* dir);

enum CHARACTER_NAMES {
  CHARACTER_ID_HUNKO = 0,
  CHARACTER_ID_GROGORIO = 1
};

/**
 * Base class for static character info
 */
class PlayerController {
 public:
  PlayerController();

  std::vector<Attack> gnd_specials;
  std::vector<Attack> air_specials;
  std::vector<Attack> gnd_normals;
  std::vector<Attack> air_normals;

  std::vector<BoxEntity> default_hurtboxes;
  std::vector<BoxEntity> default_hitboxes;

  float walking_v = 6.0;
  float backwalking_v = -4.5;
  float dash_v = 16.0;
  float dash_acc = 0.2;

  float backdash_v = -20.0;
  int f_backdash_recovery = 15;
  int f_backdash_invuln = 6;

  float fastfall_v = 3.0;
  float airstrafe_v = 4.5;

  float airdash_v = 20.0;
  int f_airdash_recovery = 15;

  float air_backdash_v = -15.0;
  int f_air_backdash_recovery = 6;
  int f_air_backdash_invuln = 4;

  float jumping_v = -30.0;
  int f_jumping_recovery = 5;

  float gravity = 1.5;
  // "Inverse rate of acceleration reduction" - https://www.dustloop.com/w/GGST/Frame_Data#Walk_and_Dash_Values
  // next_speed -= cur_speed / friction
  float friction = 50.0;

  virtual void testCharacterInclude();
  virtual std::string getStateString(const PlayerEntity* p);

  virtual int getCharacterId();

  Coordinate getPlayerCenter(PlayerEntity* p);
  virtual void updateBoxes(PlayerEntity* p);

  // Core of the state machine
  virtual void updateState(PlayerEntity* p, const ButtonStates* in);
  // Called at end of tick, applies velocity and handles collisions
  void applyMovement(PlayerEntity* p);

  // Helpers for verifying a character's state based on non-`STATE` values
  bool isActionable(PlayerEntity* p);
  bool holdingForward(const PlayerEntity* p, const ButtonStates* in);
  bool holdingBack(const PlayerEntity* p, const ButtonStates* in);

  // Used to check for a particular sequence of motions within some window of recent frames
  bool checkMotionInputs(std::vector<NumPadDir> motion, unsigned int window, const NumPadDir* buf);

  virtual void initializeAttacks();
  virtual void initializeCharacterAttrs();

  // Check for valid attacks / special attacks
  virtual bool checkAttacks(PlayerEntity* p, const ButtonStates* in, const std::vector<Attack>* attacks);
  // virtual bool checkAerialAttacks(PlayerEntity* p, const ButtonStates* in);

  // What to do while IN THIS STATE
  virtual void handleGrounded(PlayerEntity* p, const ButtonStates* in);
  virtual void handleStand(PlayerEntity* p, const ButtonStates* in);
  virtual void handleWalkForwards(PlayerEntity* p, const ButtonStates* in);
  virtual void handleWalkBackwards(PlayerEntity* p, const ButtonStates* in);
  virtual void handleDash(PlayerEntity* p, const ButtonStates* in);
  virtual void handleBackdash(PlayerEntity* p, const ButtonStates* in);
  virtual void handleCrouch(PlayerEntity* p, const ButtonStates* in);
  virtual void handleHitstun(PlayerEntity* p, const ButtonStates* in);

  virtual bool handleAerial(PlayerEntity* p, const ButtonStates* in);
  virtual void handleFall(PlayerEntity* p, const ButtonStates* in);
  virtual void handleJump(PlayerEntity* p, const ButtonStates* in);
  virtual void handleAirDash(PlayerEntity* p, const ButtonStates* in);
  virtual void handleAirBackDash(PlayerEntity* p, const ButtonStates* in);

  virtual void handleAttack(PlayerEntity* p, const ButtonStates* in);
  // virtual void handleGroundSpecial(PlayerEntity* p, const ButtonStates* in);
  // virtual void handleAirNormal(PlayerEntity* p, const ButtonStates* in);
  // virtual void handleAirSpecial(PlayerEntity* p, const ButtonStates* in);

  // Entering the state (frame 0)
  virtual void stand(PlayerEntity* p, const ButtonStates* in);
  virtual void walkForwards(PlayerEntity* p, const ButtonStates* in);
  virtual void walkBackwards(PlayerEntity* p, const ButtonStates* in);
  virtual void fall(PlayerEntity* p, const ButtonStates* in);
  virtual void dash(PlayerEntity* p, const ButtonStates* in);
  virtual void backdash(PlayerEntity* p, const ButtonStates* in);
  virtual void jump(PlayerEntity* p, const ButtonStates* in);
  virtual void airDash(PlayerEntity* p, const ButtonStates* in);
  virtual void airBackDash(PlayerEntity* p, const ButtonStates* in);
  virtual void crouch(PlayerEntity* p, const ButtonStates* in);
  // Begin an attack
  virtual void attack(PlayerEntity* p, const ButtonStates* in, const Attack* atk);
  // virtual void special(PlayerEntity* p, const ButtonStates* in, int id);

  virtual void applyAirStrafe(PlayerEntity* p, const ButtonStates* in);

  /**
   * @brief Function to adjust velocity from one speed to another 
   * @param v_cur Pointer to the current velocity, to be edited
   * @param v_start The starting velocity
   * @param v_final The goal velocity
   * @param frac Between 0 and 1. What ratio between v_start and v_final 
   * should v_cur be adjusted by.
   */
  virtual void adjustVel(float* v_cur, float v_start, float v_final, float frac);
};

struct GameScene {
  PlayerEntity players[2];  
  ButtonStates inputs[2];
};

class GameAllocator {
 public: 
  unsigned int cur_tick;
  // If online player is p1 => 0
  // If online player is p2 => 1
  // If no online player => 99
  unsigned int loc_pindex;
  unsigned int net_pindex;
  GameAllocator();
  GameAllocator(unsigned int net_p1_or_p2);

  GameScene* getCurrentScene();

  /**
   * @brief Function to move on to the next scene.
   * Copies current scene to next index, increments cur_tick,
   * and returns the *new* current scene.
   */
  GameScene* getNextScene();

  /**
   * @brief Retrieve a given player's inputs from the previous scene
   */
  const ButtonStates* getInputsAtTick(int pindex, unsigned int tick);


  /**
   * @brief Function to roll back to a given frame.
   * @param prev_tick The frame to roll back to (sets allocator's cur_tick to this)
   * @param in The inputs to insert.
   * @note After calling rollback, engine must simulate back to current frame. 
   */
  GameScene* rollBack(unsigned int prev_tick, const ButtonStates* in);

  /**
   * @brief Roll forward to the next frame, duplicating net player's input,
   * and without changing local player's previous inputs.
   */
  GameScene* rollForward();

  /**
   * @brief Used to get the numpad direction at a given tick
   * @param tick The tick to populate
   * @param pindex The player of choice
   * @note Used to populate motion buffer
   */
  void populateDirBuffer(unsigned int tick, int pindex);

  void fillCurrentMotionBuffer(ButtonStates* buf);

  GameScene* getSceneAtTick(unsigned int tick);

 private:
  GameScene history_buffer[HISTORY_BUFFER_SIZE];
  unsigned int getCurrentIndex();
  unsigned int getIndexOfTick(unsigned int tick);
};

class GameManager {
 public:
  GameAllocator allocator;
  PlayerController* players[2];

  unsigned int loc_pindex;
  unsigned int net_pindex;
  
  // local current tick
  unsigned int cur_tick;
  // Latest frame received from the remote client
  unsigned int remote_tick;
  // Last frame where game state was synchronized
  unsigned int sync_frame;
  // Latest frame advantage received from the remote client
  unsigned int remote_frame_advantage;

  GameManager();
  GameManager(PlayerController* p1, PlayerController* p2, unsigned int net_pindex);

  void updateInputs(const ButtonStates* btns, int pindex);
  void tick();
  void rollBack(unsigned int frame, const ButtonStates* in);

  PlayerEntity* getPlayer(unsigned int pid);

 private:
  void applyTickUpdates(GameScene* scene);
  void handlePlayerCollisions(PlayerEntity* p1, PlayerEntity* p2);
  void handleAttackCollisions(PlayerEntity* src, PlayerEntity* dst);
  void decrementFrames(PlayerEntity* p1);
  void setFacingDir(PlayerEntity* p1, PlayerEntity* p2);
  void setInitialPlayerPositions();
};
