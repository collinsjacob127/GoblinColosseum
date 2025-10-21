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
void printStringVec(std::vector<std::string>);