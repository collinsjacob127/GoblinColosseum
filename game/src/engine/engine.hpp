/**
 * The input system is an abstraction layer between system input and input commands
 * 
 * Based on input system designed by rcmagic (wrote GGST netcode)
 * https://github.com/rcmagic/DemoFighterWithNetcode/blob/master/game/InputSystem.lua
 */

#pragma once

#include <iostream>
#include <string.h>
#include <SDL3/SDL.h>

#define MAX_ROLLBACK_FRAMES 60
#define FRAME_ADVANTAGE_LIMIT 5
#define INITIAL_FRAME 0

#define MAX_INPUT_FRAMES 60

#define ENV_DIM_WALL_THICKNESS 20
#define ENV_DIM_FLOOR_HEIGHT 880
#define ENV_DIM_LEFT_WALL_X 0
#define ENV_DIM_RIGHT_WALL_X 1900

class InputState {
 public:
  bool up;
  bool down;
  bool left;
  bool right;
  bool attack;

  InputState();

  void reset();

  void copyFrom(const InputState *src);
};

class InputSystem {
 public:
  // uint MAX_INPUT_FRAMES = 60;

  // uint local_player_index = 1;
  // uint local_player_index = 2;

  InputState inputState;

  InputState remotePlayerState;

  // polledInput;

  // InputState playerCommandBuffer[MAX_INPUT_FRAMES];

  unsigned int inputDelay;

  InputSystem();

  void updateInputState(const SDL_Event *e);
};

class PlayerAction {
 public:
  // Frames
  int startup;
  int active;
  int recovery;

  // Hitboxes
  float x_offset;
  float y_offset;
  float x_width;
  float y_width;

  // DMG Values
  float damage;
  float hit_vel_x;
  float hit_vel_y;

  // Tags
  std::string name;

  PlayerAction();
};

// Constants that vary from character to character
class PlayerBase {
 public:
  float width;
  float height;

  PlayerAction* attack;

  // Walking speed in px / frame
  int walking_speed;
  int jumping_v0;
  float fastfall_v;

  PlayerBase();
};

// Tracks the moving parts of a player
class PlayerState {
 public: 
  float x_pos;
  float y_pos;

  int x_vel;
  int y_vel;

  int x_acc;
  int y_acc;

  int startup_frames;
  int active_frames;
  int recovery_frames;

  PlayerState(const PlayerBase* base);

  void copyFrom(const PlayerState *src);
};

class PlayerManager {
  PlayerBase* base;

  PlayerManager();

  void processInputs(const InputState *inputs, PlayerState* state);
  void computeNextState(PlayerState* state);
  // Will probably need to also pass opponent base
  bool computeCollision(PlayerState* state, PlayerState *opp_state);
};

// All essential information for the game state in a given frame
class GameScene {
 public:
  PlayerState player1;
  InputState inputs1;

  PlayerState player2;
  InputState inputs2;

  // Has this scene recieved verified input from p2?
  bool merged;
  // What game tick is this scene from
  unsigned int cur_tick;

  GameScene(const PlayerBase* pbase1, const PlayerBase* pbase2);
  
  /**
   * @brief Copies one game scene into another.
   * Meant to build a baseline to go from one frame to another.
   * Defaults to unmerged.
   * Increments tick
   */
  void copyFrom(const GameScene* src);
};

/**
 * @brief Stores a list of game scenes, handles passing from
 * one scene to the next
 */
class GameManager {
 public:
  // List of scenes
  GameScene scenes[MAX_ROLLBACK_FRAMES];
  PlayerManager p1_mgr;
  PlayerManager p2_mgr;

  // Index of the current frame
  unsigned int cur_scene_index;

  // How many ticks has the game experienced so far
  unsigned int total_ticks;
  unsigned int last_merged_tick;

  GameManager();

  // Push a new game state, updating from one to the next given inputs
  bool tick(const InputState *p1_inputs, const InputState *p2_inputs, bool rb);
  // Roll back to state at rb_tick and resimulate with updated inputs
  bool rollback(const InputState *p2_inputs, unsigned int rb_tick);

  GameScene* getCurrentScene();

 private:
  unsigned int next_index();

};


// class Entity {
//   float x_pos;
//   float y_pos;

//   float x_vel;
//   float y_vel;

//   float x_acc;
//   float y_acc;

//   float width;
//   float height;
// };
