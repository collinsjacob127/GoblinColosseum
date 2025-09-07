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
  font = TTF_OpenFont("assets/fonts/OpenSans-Regular.ttf", 24);
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
    player->width,
    player->height
  };

  SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);            // Set render draw color to green
  SDL_RenderFillRect(ren, &green_square);  // Render the rectangle

  if (player->active_frames + player->startup_frames + player->recovery_frames == 0) { return; }

  if (player->startup_frames > 0) {
    // startup
    SDL_SetRenderDrawColor(ren, 255, 247, 0, 255);     
  }
  if (player->active_frames > 0) {
    // active
    SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);     
  }
  if (player->recovery_frames > 0) {
    // recovery
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
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

void RenderEngine::displayFPS(double FPS) {
  SDL_Color color = { 120, 0, 150, 255 };

  std::stringstream ss;
  ss << std::fixed << std::setprecision(1) << FPS << std::endl;
  std::string fps_string = ss.str();
  
  // std::cout << "FPS: " << fps_string << std::endl;
  
  SDL_Surface *surface = TTF_RenderText_Solid(font, fps_string.c_str(), fps_string.size()-1, color);
  // SDL_Surface *surface = TTF_RenderText_Solid(font, "Hello Test", 10, color);
  // if (!font) { std::cerr << "Bad font\n"; }
  // if (!surface) { std::cerr << "Bad surface\n"; }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
  // if (!texture) { std::cerr << "Bad texture\n"; }

  float texW = 0, texH = 0;
  SDL_GetTextureSize(texture, &texW, &texH);

  SDL_FRect dst = {1870 - texW, 20, texW, texH};

  SDL_RenderTexture(ren, texture, NULL, &dst);

  SDL_DestroyTexture(texture);
  SDL_DestroySurface(surface);
}

void RenderEngine::clearScreen() {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer

  SDL_SetRenderDrawColor(ren, 150, 123, 68, 255); 
  SDL_FRect floor{
    0,
    ENV_DIM_FLOOR_HEIGHT,
    1920,
    200
  };
  SDL_RenderFillRect(ren, &floor);  // Render the rectangle

  SDL_SetRenderDrawColor(ren, 100, 100, 100, 255); 
  SDL_FRect left_wall{
    ENV_DIM_LEFT_WALL_X,
    0,
    ENV_DIM_WALL_THICKNESS,
    1080
  };

  SDL_FRect right_wall{
    ENV_DIM_RIGHT_WALL_X,
    0,
    ENV_DIM_WALL_THICKNESS,
    1080
  };

  SDL_RenderFillRect(ren, &left_wall);  // Render the rectangle
  SDL_RenderFillRect(ren, &right_wall);  // Render the rectangle

  // SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
}


