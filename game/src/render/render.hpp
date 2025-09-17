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
  BoxEntity local_box = {595.0, 425.0, 735.0, 126.0};
  BoxEntity online_box = {592.0, 581.0, 735.0, 126.0};
  BoxEntity settings_box = {592.0, 737.0, 735.0, 126.0};
  BoxEntity quit_box = {592.0, 893.0, 735.0, 126.0};

  SDL_FRect local_frect;
  SDL_FRect online_frect;
  SDL_FRect settings_frect;
  SDL_FRect quit_frect;

  SDL_Texture* bg_tex;
  // Selected buttons
  SDL_Texture* local_s_tex;
  SDL_Texture* online_s_tex;
  SDL_Texture* settings_s_tex;
  SDL_Texture* quit_s_tex;
  // Unselected buttons
  SDL_Texture* local_u_tex;
  SDL_Texture* online_u_tex;
  SDL_Texture* settings_u_tex;
  SDL_Texture* quit_u_tex;
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

  SDL_FRect game_border;
  // Logical view dims & location in-game
  SDL_FRect viewport;
  // Rendered view dims
  SDL_FRect output_rect;

  StartMenu start_menu;

  float scale;

  RenderEngine();
  ~RenderEngine();

  void render(const GameManager* game);
  void checkRenderDrivers();
  void renderGameScene(const GameScene* scene);
  void renderPlayer(const PlayerEntity *player);
  void displayFPS(double FPS);
  void clearScreen();
  void renderStartMenu(int selection);
  void calculateScale(int win_width, int win_height);
};
