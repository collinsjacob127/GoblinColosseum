/**
 * Headers for functions relating to rendering which interact with main.cpp
 */

#pragma once

#include <iostream>
#include <string.h>
#include <sstream>
#include <iomanip>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include "net.hpp"
#include "engine.hpp"
#include "util.hpp"


void test_render_include_works();

struct StartMenu {
  SDL_Texture* bg_tex;
  SDL_Texture* local_tex;
  SDL_Texture* online_tex;
  SDL_Texture* settings_tex;
  SDL_Texture* quit_tex;
};

struct OnlineMenu {
  SDL_Texture* prompt_tex = nullptr;
};

struct CharacterTextures {
  int character_id;
  SDL_Texture* standing;
  SDL_Texture* jumping;
  SDL_Texture* crouching;
};

class RenderEngine {
 public:
  SDL_Window* win;
  SDL_Renderer* ren;
  SDL_Texture* ren_tex;
  SDL_GPUDevice* device;

  std::vector<CharacterTextures> char_textures;

  TTF_Font* font;
  SDL_Texture* game_background;
  SDL_Texture* player_tex;
  double FPS;

  SDL_Texture* buffer_tex;
  
  SDL_FRect game_border;
  // Logical view dims & location in-game
  SDL_FRect viewport;
  // Rendered view dims
  SDL_FRect output_rect;

  StartMenu start_menu;
  OnlineMenu online_menu;

  float scale;

  RenderEngine();
  ~RenderEngine();

  void checkRenderDrivers();
  
  void loadTextureFromPath(std::string fname, SDL_Texture* tex_to_be);
  void initializeCharacterTextures(int p_index, int character_id);
  void renderStartMenu(int selection);
  void renderOnlineMenu(const NetEngine* net_engine);
  void renderGameScene(GameManager* game);
  void calculateScale(int win_width, int win_height);

 private:
  void initializeOnlineMenuPrompt();
  void renderBoxes(const PlayerEntity* p);
  void renderPlayer(const PlayerEntity *player, int p_index);
  void displayFPS();
  void clearScreen();
};
