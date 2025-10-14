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
  Timer startup_timer;
  startup_timer.start();

  game_border.x = GAME_BORDER_X0;
  game_border.y = GAME_BORDER_Y0;
  game_border.w = GAME_BORDER_X1 - GAME_BORDER_X0;
  game_border.h = GAME_BORDER_Y1 - GAME_BORDER_Y0;
  viewport.x = 0;
  viewport.y = 0;
  viewport.w = 1600;
  viewport.h = 900;

  scale = 1.0;

  // Create Window
  SDL_WindowFlags flags = {};
  flags |= SDL_WINDOW_OPENGL;
  // flags |= SDL_WINDOW_BORDERLESS;
  // flags |= SDL_WINDOW_FULLSCREEN;
  win = SDL_CreateWindow("Goblin Colosseum", 1920, 1080, flags);
  if (win == nullptr) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(EXIT_FAILURE);
  } else { std::cout << "Window Created\n";}

  // Create Renderer
  // ren = SDL_CreateRenderer(win, "gpu");
  ren = SDL_CreateRenderer(win, NULL);
  if (ren == nullptr) {
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    exit(EXIT_FAILURE);
  } else { std::cout << "Renderer set\n";}
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  
  buffer_tex = SDL_CreateTexture(
    ren, 
    SDL_PIXELFORMAT_ARGB32,
    SDL_TEXTUREACCESS_TARGET,
    DEFAULT_XDIM, DEFAULT_YDIM);

  // Load a font
  font = TTF_OpenFont("assets/fonts/pixel-mono/editundo.ttf", 24);
  if (!font) {
    std::cerr << "Font load error: " << SDL_GetError() << std::endl;
    exit(EXIT_FAILURE);
  } else { std::cout << "Font Loaded\n"; }

  /**
   * INITIALIZING ASSETS FOR START MENU
   */
  // Background
  SDL_Surface* start_bg_png = IMG_Load("assets/backgrounds/start/none-selected.png");
  // Selected buttons
  SDL_Surface* local_s_png = IMG_Load("assets/backgrounds/start/local-selected.png");
  SDL_Surface* online_s_png = IMG_Load("assets/backgrounds/start/online-selected.png");
  SDL_Surface* settings_s_png = IMG_Load("assets/backgrounds/start/settings-selected.png");
  SDL_Surface* quit_s_png = IMG_Load("assets/backgrounds/start/quit-selected.png");
  // Verify read correctly
  if (start_bg_png == nullptr || local_s_png == nullptr || online_s_png == nullptr
      || settings_s_png == nullptr || quit_s_png == nullptr) {
    std::cerr << "failed to load png\n";
  }
  // background
  start_menu.bg_tex = SDL_CreateTextureFromSurface(ren, start_bg_png);
  // selected buttons
  start_menu.local_tex = SDL_CreateTextureFromSurface(ren, local_s_png);
  start_menu.online_tex = SDL_CreateTextureFromSurface(ren, online_s_png);
  start_menu.settings_tex = SDL_CreateTextureFromSurface(ren, settings_s_png);
  start_menu.quit_tex = SDL_CreateTextureFromSurface(ren, quit_s_png);

  SDL_DestroySurface(start_bg_png);
  SDL_DestroySurface(local_s_png);
  SDL_DestroySurface(online_s_png);
  SDL_DestroySurface(settings_s_png);
  SDL_DestroySurface(quit_s_png);

  SDL_Surface* game_bg_png = IMG_Load("assets/backgrounds/game/background.png");
  game_background = SDL_CreateTextureFromSurface(ren, game_bg_png);
  SDL_DestroySurface(game_bg_png);

  SDL_Surface* gob0_png = IMG_Load("assets/characters/gob0/GOB0.png");
  player_tex = SDL_CreateTextureFromSurface(ren, gob0_png);
  SDL_DestroySurface(gob0_png);
  std::cout << "Background and character assets loaded\n";

  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
  // SDL_GL_SetSwapInterval(2);
  std::cout << "Renderer finished initializing (" << startup_timer.duration() << "s)\n";
}

RenderEngine::~RenderEngine() {
  // Cleanup menu textures
  SDL_DestroyTexture(start_menu.bg_tex);
  SDL_DestroyTexture(start_menu.local_tex);
  SDL_DestroyTexture(start_menu.online_tex);
  SDL_DestroyTexture(start_menu.settings_tex);
  SDL_DestroyTexture(start_menu.quit_tex);
  SDL_DestroyTexture(game_background);
  SDL_DestroyTexture(player_tex);
  SDL_DestroyTexture(buffer_tex);

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

void RenderEngine::renderBoxes(const PlayerEntity* p) {
  // Hitboxes red
  SDL_FRect hitbox;
  SDL_SetRenderDrawColor(ren, 255, 0, 0, 120);     
  for (size_t i = 0; i < p->hitboxes.size(); ++i) {
    convertBoxEntityToFRect(&p->hitboxes.at(i), &hitbox);
    // if (ENABLE_BOX_DEBUG)
    //   std::cout << "Rendering hitbox at " << hitbox.x << ", " << hitbox.y << std::endl;
    SDL_RenderFillRect(ren, &hitbox);
  }

  // Hurtboxes based on state
  if (p->f_startup + p->f_active + p->f_recovery) {
    // Display as non-actionable (yellow)
    SDL_SetRenderDrawColor(ren, 255, 255, 0, 50);     
  } else {
    // Display as actionable (green)
    SDL_SetRenderDrawColor(ren, 0, 255, 0, 50);     
  }
  if (p->state == HITSTUN) {
    // Display as hitstun (orange)
    SDL_SetRenderDrawColor(ren, 255, 115, 5, 50);     
  }
  SDL_FRect hurtbox;
  for (size_t i = 0; i < p->hurtboxes.size(); ++i) {
    convertBoxEntityToFRect(&p->hurtboxes.at(i), &hurtbox);
    // if (ENABLE_BOX_DEBUG)
    //   std::cout << "Rendering hurtbox at " << hurtbox.x << ", " << hurtbox.y << std::endl;
    SDL_RenderFillRect(ren, &hurtbox);
  }

}

void RenderEngine::renderPlayer(const PlayerEntity *p) {
  SDL_FRect green_square{ 
    p->x_pos,
    p->y_pos, 
    p->width,
    p->height
  };

  // Rendering the player's texture
  if (p->facing_right) {
    SDL_RenderTexture(ren, player_tex, NULL, &green_square);
  } else {
    SDL_RenderTextureRotated(ren, player_tex, NULL, &green_square, 0, NULL, SDL_FLIP_HORIZONTAL);
  }
}

void RenderEngine::displayFPS() {

  SDL_Color color = { 255, 255, 255, 255 };
  // SDL_Color color = { 120, 0, 150, 255 };

  std::stringstream ss;
  ss << std::fixed << std::setprecision(0) << FPS << std::endl;
  std::string fps_string = ss.str();
  
  SDL_Surface *surface = TTF_RenderText_Solid(font, fps_string.c_str(), fps_string.size()-1, color);
  if (!surface) { std::cerr << "Bad surface\n"; }

  SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, surface);
  if (!texture) { std::cerr << "Bad texture\n"; }

  float texW = 0, texH = 0;
  SDL_GetTextureSize(texture, &texW, &texH);

  int ren_w, ren_h;
  SDL_GetCurrentRenderOutputSize(ren, &ren_w, &ren_h);

  SDL_FRect dst = {ren_w - (texW*scale*(float)1.5), 20*scale, texW*scale, texH*scale};
  SDL_RenderTexture(ren, texture, NULL, &dst);

  SDL_DestroyTexture(texture);
  SDL_DestroySurface(surface);
}

void RenderEngine::clearScreen() {
  SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);  // Set render draw color to black
  SDL_RenderClear(ren);         // Clear the renderer
}


void RenderEngine::renderStartMenu(int selection) {
  clearScreen(); // Clears window
  // Render to a temporary texture first
  SDL_SetRenderTarget(ren, buffer_tex);
  clearScreen(); // Clears texture
  switch (selection) {
    case 0:
      SDL_RenderTexture(ren, start_menu.local_tex, NULL, NULL);
      break;
    case 1:
      SDL_RenderTexture(ren, start_menu.online_tex, NULL, NULL);
      break;
    case 2:
      SDL_RenderTexture(ren, start_menu.settings_tex, NULL, NULL);
      break;
    case 3:
      SDL_RenderTexture(ren, start_menu.quit_tex, NULL, NULL);
      break;
    default:
      SDL_RenderTexture(ren, start_menu.bg_tex, NULL, NULL);
      break;
  }

  // Set target back to window and render the buffer
  SDL_SetRenderTarget(ren, NULL);
  SDL_RenderTexture(ren, buffer_tex, NULL, NULL);
  displayFPS();
  SDL_RenderPresent(ren);
}


void RenderEngine::renderGameScene(GameManager* game) {
  const GameScene* scene = game->allocator.getCurrentScene();
  const PlayerEntity* p1 = &scene->players[0];
  const PlayerEntity* p2 = &scene->players[1];

  // int win_w, win_h;
  // if (!SDL_GetWindowSize(win, &win_w, &win_h)) {
  //   std::cerr << SDL_GetError() << std::endl;
  // }
  // calculateScale(win_w, win_h);

  // Currently: Centers between two players, fixed size
  viewport.x = (p1->x_pos + p2->x_pos) / 2;
  viewport.x = viewport.x > 3200 - viewport.w/2 ? 3200 - viewport.w/2 : viewport.x;
  viewport.x -= viewport.w / 2;
  viewport.x = viewport.x < 0 ? 0 : viewport.x;
  viewport.y = 900;

  // Render full scene to temp buffer
  SDL_SetRenderTarget(ren, buffer_tex);
  // clearScreen();
  SDL_RenderTexture(ren, game_background, NULL, NULL);

  renderBoxes(p1);
  renderPlayer(p1);

  renderBoxes(p2);
  renderPlayer(p2);

  // Render viewport selection to window
  SDL_SetRenderTarget(ren, NULL);
  // clearScreen();
  SDL_RenderTexture(ren, buffer_tex, &viewport, NULL);
  displayFPS();
  SDL_RenderPresent(ren);
}
