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

  game_border.x = GAME_BORDER_X0;
  game_border.y = GAME_BORDER_Y0;
  game_border.w = GAME_BORDER_X1 - GAME_BORDER_X0;
  game_border.h = GAME_BORDER_Y1 - GAME_BORDER_Y0;
  viewport.x = 0;
  viewport.y = 0;
  viewport.w = 1920;
  viewport.h = 1080;

  // Create Window
  SDL_WindowFlags flags = {};
  flags |= SDL_WINDOW_OPENGL;
  flags |= SDL_WINDOW_BORDERLESS;
  flags |= SDL_WINDOW_FULLSCREEN;
  win = SDL_CreateWindow("Goblin Colosseum", 1920, 1080, flags);
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
  
  // Bind GPU
  // device = SDL_CreateGPUDevice( SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL, true, NULL);
  // if (!device) {
  //   std::cerr << "\nSDL Failed to capture GPU Device" << std::endl;
  // }
  // SDL_ClaimWindowForGPUDevice(device, win);
  
  // ren_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB32);

  // Load a font
  font = TTF_OpenFont("assets/fonts/pixel-mono/editundo.ttf", 24);
  if (!font) {
    std::cerr << "Font load error: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  }

  /**
   * INITIALIZING ASSETS FOR START MENU
   */
  // Background
  SDL_Surface* start_bg_png = IMG_Load("assets/backgrounds/start/title-background.png");
  // Selected buttons
  SDL_Surface* local_s_png = IMG_Load("assets/backgrounds/start/local-selected.png");
  SDL_Surface* online_s_png = IMG_Load("assets/backgrounds/start/online-selected.png");
  SDL_Surface* settings_s_png = IMG_Load("assets/backgrounds/start/settings-selected.png");
  SDL_Surface* quit_s_png = IMG_Load("assets/backgrounds/start/quit-selected.png");
  // Unselected buttons
  SDL_Surface* local_u_png = IMG_Load("assets/backgrounds/start/local-unselected.png");
  SDL_Surface* online_u_png = IMG_Load("assets/backgrounds/start/online-unselected.png");
  SDL_Surface* settings_u_png = IMG_Load("assets/backgrounds/start/settings-unselected.png");
  SDL_Surface* quit_u_png = IMG_Load("assets/backgrounds/start/quit-unselected.png");
  // Verify read correctly
  if (start_bg_png == nullptr || local_s_png == nullptr || online_s_png == nullptr
      || settings_s_png == nullptr || quit_s_png == nullptr || local_u_png == nullptr ||
      settings_u_png == nullptr || online_u_png == nullptr || quit_u_png == nullptr) {
    std::cerr << "failed to load png\n";
  }
  // background
  start_menu.bg_tex = SDL_CreateTextureFromSurface(ren, start_bg_png);
  // selected buttons
  start_menu.local_s_tex = SDL_CreateTextureFromSurface(ren, local_s_png);
  start_menu.online_s_tex = SDL_CreateTextureFromSurface(ren, online_s_png);
  start_menu.settings_s_tex = SDL_CreateTextureFromSurface(ren, settings_s_png);
  start_menu.quit_s_tex = SDL_CreateTextureFromSurface(ren, quit_s_png);
  // unselected buttons
  start_menu.local_u_tex = SDL_CreateTextureFromSurface(ren, local_u_png);
  start_menu.online_u_tex = SDL_CreateTextureFromSurface(ren, online_u_png);
  start_menu.settings_u_tex = SDL_CreateTextureFromSurface(ren, settings_u_png);
  start_menu.quit_u_tex = SDL_CreateTextureFromSurface(ren, quit_u_png);

  SDL_DestroySurface(start_bg_png);
  SDL_DestroySurface(local_s_png);
  SDL_DestroySurface(local_u_png);
  SDL_DestroySurface(online_s_png);
  SDL_DestroySurface(online_u_png);
  SDL_DestroySurface(settings_s_png);
  SDL_DestroySurface(settings_u_png);
  SDL_DestroySurface(quit_s_png);
  SDL_DestroySurface(quit_u_png);
  convertBoxEntityToFRect(&start_menu.local_box, &start_menu.local_frect);
  convertBoxEntityToFRect(&start_menu.online_box, &start_menu.online_frect);
  convertBoxEntityToFRect(&start_menu.settings_box, &start_menu.settings_frect);
  convertBoxEntityToFRect(&start_menu.quit_box, &start_menu.quit_frect);

  SDL_Surface* game_bg_png = IMG_Load("assets/backgrounds/game/background.png");
  game_background = SDL_CreateTextureFromSurface(ren, game_bg_png);
  SDL_DestroySurface(game_bg_png);

  SDL_Surface* gob0_png = IMG_Load("assets/characters/gob0/GOB0.png");
  player_tex = SDL_CreateTextureFromSurface(ren, gob0_png);
  SDL_DestroySurface(gob0_png);

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
}

RenderEngine::~RenderEngine() {
  // Cleanup menu textures
  SDL_DestroyTexture(start_menu.bg_tex);
  SDL_DestroyTexture(start_menu.local_s_tex);
  SDL_DestroyTexture(start_menu.local_u_tex);
  SDL_DestroyTexture(start_menu.online_s_tex);
  SDL_DestroyTexture(start_menu.online_u_tex);
  SDL_DestroyTexture(start_menu.settings_s_tex);
  SDL_DestroyTexture(start_menu.settings_u_tex);
  SDL_DestroyTexture(start_menu.quit_s_tex);
  SDL_DestroyTexture(start_menu.quit_u_tex);

  // SDL_DestroyGPUDevice(device);
  SDL_DestroyRenderer(ren);
  SDL_DestroyWindow(win);
  SDL_Quit();
}

void RenderEngine::calculateScale(int win_width, int win_height) {
  float scale_x = (float) win_width / viewport.w;
  float scale_y = (float) win_height / viewport.h;
  scale = SDL_min(scale_x, scale_y);
  output_rect.w = viewport.w * scale;
  output_rect.h = viewport.h * scale;
  // Unsure about this part
  output_rect.x = (win_width - output_rect.w) / 2.0f;
  output_rect.y = (win_height - output_rect.h) / 2.0f;
}

void RenderEngine::checkRenderDrivers() {
  int n_drivers = SDL_GetNumRenderDrivers();
  for (int i = 0; i < n_drivers; i++) {
    std::cout << SDL_GetRenderDriver(i) << "\n";
  }
}

void RenderEngine::renderPlayer(const PlayerEntity *p) {
  SDL_FRect green_square{ 
    p->x_pos,
    p->y_pos, 
    p->width,
    p->height
  };
  // // Set render draw color to green
  // SDL_SetRenderDrawColor(ren, p->disp_r, p->disp_g, p->disp_b, 255);            
  // // Render the rectangle
  // SDL_RenderFillRect(ren, &green_square);  
  SDL_RenderTexture(ren, player_tex, NULL, &green_square);

  if (p->f_active + p->f_startup + p->f_recovery == 0) { return; }

  if (p->f_startup > 0) {
    // startup
    SDL_SetRenderDrawColor(ren, 255, 247, 0, 255);     
  }
  if (p->f_active > 0) {
    // active
    SDL_SetRenderDrawColor(ren, 0, 0, 255, 255);     
  }
  if (p->f_recovery > 0) {
    // recovery
    SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
  }


  // Active frames > 0
  SDL_FRect atk_square{
    p->x_pos + p->width,
    p->y_pos + p->height / 4,
    100,
    50,
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

  SDL_FRect dst = {output_rect.x+output_rect.w - (texW*scale*(float)1.5), 20*scale, texW*scale, texH*scale};

  SDL_RenderTexture(ren, texture, NULL, &dst);

  SDL_DestroyTexture(texture);
  SDL_DestroySurface(surface);
}

void RenderEngine::clearScreen() {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
}


void RenderEngine::renderStartMenu(int selection) {
  SDL_RenderTexture(ren, start_menu.bg_tex, NULL, NULL);
  // Local is hovered
  if (selection == 0) {
    SDL_RenderTexture(ren, start_menu.local_s_tex, NULL, &start_menu.local_frect);
  } else {
    SDL_RenderTexture(ren, start_menu.local_u_tex, NULL, &start_menu.local_frect);
  }
  
  // Online is hovered
  if (selection == 1) {
    SDL_RenderTexture(ren, start_menu.online_s_tex, NULL, &start_menu.online_frect);
  } else {
    SDL_RenderTexture(ren, start_menu.online_u_tex, NULL, &start_menu.online_frect);
  }
  // Settings is hovered
  if (selection == 2) {
    SDL_RenderTexture(ren, start_menu.settings_s_tex, NULL, &start_menu.settings_frect);
  } else {
    SDL_RenderTexture(ren, start_menu.settings_u_tex, NULL, &start_menu.settings_frect);
  }
  // Quit is hovered
  if (selection == 3) {
    SDL_RenderTexture(ren, start_menu.quit_s_tex, NULL, &start_menu.quit_frect);
  } else {
    SDL_RenderTexture(ren, start_menu.quit_u_tex, NULL, &start_menu.quit_frect);
  }
}

void RenderEngine::renderGameScene(const GameScene* scene) {
  const PlayerEntity* p1 = &scene->players[0];
  const PlayerEntity* p2 = &scene->players[1];

  viewport.x = (p1->x_pos + p2->x_pos) / 2;
  viewport.y = 1080;

  SDL_RenderTexture(ren, game_background, NULL, NULL);
  renderPlayer(p1);
  renderPlayer(p2);

}
