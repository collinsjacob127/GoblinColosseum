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
