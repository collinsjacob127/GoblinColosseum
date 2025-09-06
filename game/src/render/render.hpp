/**
 * Headers for functions relating to rendering which interact with main.cpp
 */

#pragma once

#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "engine.hpp"

void test_render_include_works();

class RenderEngine {
 public:
  SDL_Window* win;
  SDL_Renderer* ren;
  TTF_Font* font;

  RenderEngine();

  void checkRenderDrivers();
  void renderPlayer(const PlayerState *player);
  void clearScreen();

};
