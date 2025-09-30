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

#define ENABLE_HELPER_PRINTOUTS true

// For rollback functionality demo:
#define MAX_ROLLBACK_FRAMES 60
#define MAX_INPUT_FRAMES 60
#define INITIAL_FRAME 60
#define HISTORY_BUFFER_SIZE 120

#define FRAME_ADVANTAGE_LIMIT 5


#define DEFAULT_XDIM 3200
#define DEFAULT_YDIM 1800

#define GAME_BORDER_X0 180
#define GAME_BORDER_X1 3020
#define GAME_BORDER_Y0 0
#define GAME_BORDER_Y1 1678

struct Keybinds {
  SDL_Scancode up = SDL_SCANCODE_W;
  SDL_Scancode down = SDL_SCANCODE_S;
  SDL_Scancode left = SDL_SCANCODE_A;
  SDL_Scancode right = SDL_SCANCODE_D;
  SDL_Scancode b1 = SDL_SCANCODE_U; // ps square
  SDL_Scancode b2 = SDL_SCANCODE_I; // ps triangle
  SDL_Scancode b3 = SDL_SCANCODE_J; // ps X
  SDL_Scancode b4 = SDL_SCANCODE_K; // ps circle
  SDL_Scancode l1 = SDL_SCANCODE_O; // left bumper
  SDL_Scancode r1 = SDL_SCANCODE_L; // right bumper
  // SDL_Scancode l2 = SDL_SCANCODE_P; // left trigger
  SDL_Scancode l2 = SDL_SCANCODE_LSHIFT; // left trigger
  SDL_Scancode r2 = SDL_SCANCODE_SEMICOLON; // right trigger
};

enum Button {
  PRESSED = 2, // Pressed this frame
  HELD = 1,    // Still being held
  RELEASED = 0 // Released
};

enum NumPadDir {
  DOWN_LEFT = 1,
  DOWN = 2,
  DOWN_RIGHT = 3,
  LEFT = 4,
  CENTER = 5,
  RIGHT = 6,
  UP_LEFT = 7,
  UP = 8,
  UP_RIGHT = 9,
  MISC = 10,
};

enum ButtonName {
  B1,
  B2,
  B3,
  B4,
  L1,
  R1,
  L2,
  R2
};

struct ButtonStates {
  Button up = RELEASED;
  Button down = RELEASED;
  Button left = RELEASED;
  Button right = RELEASED;
  // PS square
  Button b1 = RELEASED; 
  // ps triangle
  Button b2 = RELEASED; 
  // ps X
  Button b3 = RELEASED; 
  // ps circle
  Button b4 = RELEASED; 
  Button l1 = RELEASED;
  Button r1 = RELEASED;
  Button l2 = RELEASED;
  Button r2 = RELEASED;
  NumPadDir dir_buffer[MAX_INPUT_FRAMES];
};

// Used for debugging
std::string printButtonState(const Button* btn);
void printMotionBuffer(NumPadDir* dir_buffer);
void showButtonStates(const ButtonStates* btn_state);
bool isAnyButtonPressed(const ButtonStates* in);
Button getButtonState(const ButtonStates* in, ButtonName b);

class InputSystem {
 public:
  ButtonStates buttons;
  Keybinds bindings;

  InputSystem();
  // Directly sets buttons to pressed or unpressed in the current allocation
  void updateButtonStates(const SDL_Event *e);
  void resetButtonStates();
  void setP2DefaultBindings();
 private:
};

void applyButtonUpdate(const Button* prev_btn, Button* cur_btn);

/** 
 * @brief Differentiates between pressed and held button states based on previous state
 * @param prev_buttons Pointer to the reference button state
 * @param cur_buttons Pointer to the current button state, to be updated based on the previous.
 * @note Updates current tick's button state based on the previous button state.
 * ONLY USE ONCE PER TICK
 */
void handleButtonStateTick(const ButtonStates* prev_buttons, ButtonStates* cur_buttons);

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
  AIR_SPECIAL
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

  // i.e., AIR_NORMAL, GROUND_NORMAL, AIR_SPECIAL, or GROUND_SPECIAL
  State state;
  std::string name;

  // Define button used to launch attack
  ButtonName button;

  // Specials have motions
  std::vector<NumPadDir> motion; 
  // How many frames back should this check
  unsigned int f_window;

  // Normals have directions
  NumPadDir dir;
  
  // Hitboxes and hurtboxes associated with this attack
  std::vector<BoxEntity>* getHitboxes();
  std::vector<BoxEntity> hitboxes;

  std::vector<BoxEntity>* getHurtboxes();
  std::vector<BoxEntity> hurtboxes;

  unsigned int f_startup;
  unsigned int f_active;
  unsigned int f_recovery;
};


/**
 * Struct for dynamic character info
 */
struct PlayerEntity {
  State state;
  const Attack* cur_attack;

  float v_mod = 1.0;

  bool block = false;

  float x_pos = 1920.0/2.0;
  float y_pos = 1080.0/2.0;

  bool facing_right = true;

  float x_vel = 0.0;
  float y_vel = 0.0;

  float width = 100.0;
  float height = 300.0;

  int air_action_cnt = 0;
  int air_action_max = 2;

  int f_startup = 0;
  int f_active = 0;
  int f_recovery = 0;

  int f_invuln = 0;
  int f_hitstun = 0;
};

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

  float walking_v = 6.0;
  float backwalking_v = -4.5;
  float dash_v = 16.0;
  float dash_acc = 0.4;

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

  float jumping_v = -35.0;
  int f_jumping_recovery = 5;

  float gravity = 2.5;
  // "Inverse rate of acceleration reduction" - https://www.dustloop.com/w/GGST/Frame_Data#Walk_and_Dash_Values
  // next_speed -= cur_speed / friction
  float friction = 12.0;


  virtual void testCharacterInclude();
  virtual std::string getStateString(const PlayerEntity* p);

  virtual void updateState(PlayerEntity* p, const ButtonStates* in);
  void applyMovement(PlayerEntity* p);

  bool isActionable(PlayerEntity* p);
  bool isGrounded(PlayerEntity* p);
  bool holdingForward(const PlayerEntity* p, const ButtonStates* in);
  bool holdingBack(const PlayerEntity* p, const ButtonStates* in);

  // Used to check for a particular sequence of motions within some window of recent frames
  bool checkMotionInputs(std::vector<NumPadDir> motion, unsigned int window, const NumPadDir* buf);

 private:

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

  virtual void handleAerial(PlayerEntity* p, const ButtonStates* in);
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

 private:
  GameScene history_buffer[HISTORY_BUFFER_SIZE];
  unsigned int getCurrentIndex();
  unsigned int getIndexOfTick(unsigned int tick);
  GameScene* getSceneAtTick(unsigned int tick);
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
  GameManager(unsigned int net_p1_or_p2);

  void updateInputs(const ButtonStates* btns, int pindex);
  void tick();
  void rollBack(unsigned int frame, const ButtonStates* in);

  PlayerEntity* getPlayer(unsigned int pid);

 private:
  void applyTickUpdates(GameScene* scene);
  void decrementFrames(PlayerEntity* p1);
  void setFacingDir(PlayerEntity* p1, PlayerEntity* p2);
  void setInitialPlayerPositions();
};
