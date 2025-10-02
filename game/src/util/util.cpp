/**
 * Definitions for helper functions
 */

#include "util.hpp"

/***************************
 ********* TIMER ***********
 ***************************/
Timer::Timer() {
  has_started = false;
}

void Timer::start() {
  start_time = std::chrono::high_resolution_clock::now();
  has_started = true;
}

double Timer::duration() {
    if (!has_started) {
      std::cerr << "Warning: Timer has not been started yet." << std::endl;
      return -1;
    }
    auto now = std::chrono::high_resolution_clock::now();
    // Calculate the duration in nanoseconds.
    auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - start_time)
            .count();
    // Convert nanoseconds to seconds as a long double.
    return static_cast<double>(ns) / 1e9;
}

bool checkBoxPointCollision(const Coordinate* coord, const BoxEntity* box) {
  return (coord->x <= box->x + box->width && coord->x >= box->x) && \
         (coord->y <= box->y + box->height && coord->y >= box->y);
}

void convertBoxEntityToFRect(const BoxEntity* box, SDL_FRect* frect) {
  frect->x = box->x;
  frect->y = box->y;
  frect->h = box->height;
  frect->w = box->width;
}

void rescaleBox(float pos_scale, float size_scale, BoxEntity* box) {
  box->x *= pos_scale;
  box->y *= pos_scale;
  box->width *= size_scale;
  box->height *= size_scale;
}

std::ostream& operator<<(std::ostream& os, const BoxEntity& obj) {
  os << "x: " << obj.x << ", ";
  os << "y: " << obj.y << ", ";
  os << "w: " << obj.width << ", ";
  os << "h: " << obj.height;
  return os;
}

Coordinate getBoxCenterCoordinate(const BoxEntity* box) {
  Coordinate pnt;
  pnt.x = box->x + box->width / 2;
  pnt.y = box->y + box->height / 2;
  return pnt;
}

void setBoxCenterCoordinate(BoxEntity* box, Coordinate pnt) {
  box->x = pnt.x + box->width / 2;
  box->y = pnt.y + box->height / 2;
}
