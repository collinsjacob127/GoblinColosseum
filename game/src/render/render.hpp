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


class RenderEngine {
 public:
  SDL_Window* win;
  SDL_Renderer* ren;
  SDL_Texture* ren_tex;
  SDL_GPUDevice* device;

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

  float scale;

  RenderEngine();
  ~RenderEngine();

  void checkRenderDrivers();
  

  void renderGameScene(GameManager* game);
  void renderStartMenu(int selection);
  void calculateScale(int win_width, int win_height);

 private:
  void renderPlayer(const PlayerEntity *player);
  void displayFPS();
  void clearScreen();
};
