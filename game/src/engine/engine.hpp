/**
 * Headers for functions relating to rendering which interact with main.cpp
 */

#pragma once

#include <iostream>

void test_engine_include_works();

class Inputs {
 public:
  bool up;
  bool down;
  bool left;
  bool right;

  Inputs();
};


