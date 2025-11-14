/**
 * Definitions for helper functions
 */

#include "util.hpp"

/***************************
 ********* PRINT FMT ***********
 ***************************/

void CoutEscapes::printInColor(std::string message, std::string color) {
  std::cout << std::flush << color;
  std::cout << message;
  std::cout << white_fg;
}

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

BoxEntity::BoxEntity() {};
BoxEntity::BoxEntity(float x_, float y_, float w_, float h_) {
  x = x_;
  y = y_;
  width = w_;
  height = h_;
}

bool checkPointInRange(float x, float x0, float x1) {
  return (x >= x0 && x <= x1);
}

// Replace with this:
// A.x < B.x + B.width && A.x + A.width > B.x 
// && A.y < B.y + B.height && A.y + A.height > B.y
bool BoxEntity::checkCollision(BoxEntity* box) {
  return (
      x < box->x + box->width && x + width > box->x
    ) && (
      y < box->y + box->height && y + height > box->y
    );
}

Coordinate BoxEntity::getCenter() {
  Coordinate coord;
  coord.x = x + width/2;
  coord.y = y + height/2;
  return coord;
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

/**
 * STRING UTILS
 */

void printStringVec(std::vector<std::string> vec) {
  for (size_t i = 0; i < vec.size(); ++i) {
    std::cout << "  [" << i << "] ";
    std::cout << vec.at(i) << std::endl;
  }
}
