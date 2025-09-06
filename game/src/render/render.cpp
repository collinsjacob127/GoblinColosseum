/**
 * Definitions for rendering functions
 */

#include "render.hpp"

void test_render_include_works() {
    std::cout << "Include works!\n";
}

RenderEngine::RenderEngine() {
  SDL_Init(SDL_INIT_VIDEO);
  TTF_Init();

  // Create Window
  win = SDL_CreateWindow("Goblin Colosseum", 1920, 1080, SDL_WINDOW_OPENGL);
  if (win == nullptr) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  // Create Renderer
  ren = SDL_CreateRenderer(win, "gpu");
  if (ren == nullptr) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_DestroyWindow(win);
    SDL_Quit();
    exit(EXIT_FAILURE);
  }

  // Load a font
  TTF_Font* font = TTF_OpenFont("build/assets/fonts/OpenSans-Regular.ttf", 24);
  if (!font) {
    std::cerr << "Font load error: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
}

void RenderEngine::checkRenderDrivers() {
  int n_drivers = SDL_GetNumRenderDrivers();
  for (int i = 0; i < n_drivers; i++) {
    std::cout << SDL_GetRenderDriver(i) << "\n";
  }
}

void RenderEngine::renderPlayer(const PlayerState *player) {
  SDL_FRect green_square{ 
    player->x_pos,
    player->y_pos, 
    100,
    100
  };

  SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);            // Set render draw color to green
  SDL_RenderFillRect(ren, &green_square);  // Render the rectangle

  if (player->active_frames + player->startup_frames + player->recovery_frames == 0) { return; }

  if (player->startup_frames > 0) {
    SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);            // Set render draw color to green
  }
  if (player->active_frames > 0) {
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);            // Set render draw color to green
  }
  if (player->recovery_frames > 0) {
    SDL_SetRenderDrawColor(ren, 255, 0, 255, 255);            // Set render draw color to green
  }

  // Active frames > 0
  SDL_FRect atk_square{
    player->x_pos + player->attack.x_offset,
    player->y_pos + player->attack.y_offset,
    player->attack.x_width,
    player->attack.y_width,
  };

  SDL_RenderFillRect(ren, &atk_square);  // Render the rectangle
}

void RenderEngine::clearScreen() {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  // SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
}


