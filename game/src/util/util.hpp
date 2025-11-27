/**
 * Helper function headers
 */

#pragma once

#include <chrono>      // Timer :)
#include <iostream>    // User I/O
#include <sstream>     // String formatting
#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

/***************************
 ********* PRINT FMT ***********
 ***************************/
struct CoutEscapes {
  std::string black_fg = "\x1b[30m";
  std::string red_fg = "\x1b[31m";
  std::string green_fg = "\x1b[32m";
  std::string yellow_fg = "\x1b[33m";
  std::string blue_fg = "\x1b[34m";
  std::string magenta_fg = "\x1b[35m";
  std::string cyan_fg = "\x1b[36m";
  std::string white_fg = "\x1b[37m";

  std::string brt_black_fg = "\x1b[90m";
  std::string brt_red_fg = "\x1b[91m";
  std::string brt_green_fg = "\x1b[92m";
  std::string brt_yellow_fg = "\x1b[93m";
  std::string brt_blue_fg = "\x1b[94m";
  std::string brt_magenta_fg = "\x1b[95m";
  std::string brt_cyan_fg = "\x1b[96m";
  std::string brt_white_fg = "\x1b[97m";

  std::string black_bg = "\x1b[40m";
  std::string red_bg = "\x1b[41m";
  std::string green_bg = "\x1b[42m";
  std::string yellow_bg = "\x1b[43m";
  std::string blue_bg = "\x1b[44m";
  std::string magenta_bg = "\x1b[45m";
  std::string cyan_bg = "\x1b[46m";
  std::string white_bg = "\x1b[47m";

  std::string brt_black_bg = "\x1b[100m";
  std::string brt_red_bg = "\x1b[101m";
  std::string brt_green_bg = "\x1b[102m";
  std::string brt_yellow_bg = "\x1b[103m";
  std::string brt_blue_bg = "\x1b[104m";
  std::string brt_magenta_bg = "\x1b[105m";
  std::string brt_cyan_bg = "\x1b[106m";
  std::string brt_white_bg = "\x1b[107m";

  std::string line_reset = "\r\033[K";
  std::string carriage_return = "\r";

  /**
   * @brief Prints message in color, then returns cout to white
   */
  void printInColor(std::string message, std::string color);

  /**
   * @brief Prints the provided error message in red
   */
  void printError(std::string message);

  /**
   * @brief Prints the provided warning message in yellow
   */
  void printWarning(std::string message);

  /**
   * @brief Prints the provided warning message in green
   */
  void printSuccess(std::string message);
};
static CoutEscapes COLORS;

std::string getBinaryString(char input, size_t n_bytes);

/***************************
 ********* TIMER ***********
 ***************************/

/**
 * @brief Timer class for tracking duration of events.
 * @note Basically just a wrapper for the chrono high resolution
 * clock.
 */
class Timer {
 public:
  // Constructor initializes the flag to false.
  Timer();

  // Start the timer by capturing the current high-resolution time.
  void start();

  /**
   * @brief Function to return the duration of time (seconds) since
   * `Timer::start()` was last called.
   * @return The time in seconds, with ns precision.
   */
  double duration();

 private:
  // The time when start() was last called.
  std::chrono::high_resolution_clock::time_point start_time;
  // Flag to indicate whether the timer has been started.
  bool has_started;
};

void crossPlatformSleep(uint32_t milliseconds);

struct Coordinate {
  float x = 0.0;
  float y = 0.0;
};

struct BoxEntity {
  float x = 0.0;
  float y = 0.0;
  float width = 10.0;
  float height = 10.0;

  BoxEntity();
  BoxEntity(float x_, float y_, float w_, float h_);

  bool checkCollision(BoxEntity* box);
  Coordinate getCenter();
  friend std::ostream& operator<<(std::ostream& os, const BoxEntity& obj);
};

bool checkPointInRange(float x, float x0, float x1);

bool checkBoxPointCollision(const Coordinate* coord, const BoxEntity* box);
void convertBoxEntityToFRect(const BoxEntity* box, SDL_FRect* frect);
void rescaleBox(float pos_scale, float size_scale, BoxEntity* box);
Coordinate getBoxCenterCoordinate(const BoxEntity* box);
void setBoxCenterCoordinate(BoxEntity* box, Coordinate pnt);

/**
 * STRING UTILS
 */

/**
 * @brief Helper function to print a vector of strings
 */
void printStringVec(std::vector<std::string> vec);

