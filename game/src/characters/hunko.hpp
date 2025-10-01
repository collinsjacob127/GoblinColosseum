/**
 * Author: Jacob Collins
 * Description:
 * This file has the PlayerController subclass for Hunko, the Brutish
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <string.h>
#include <SDL3/SDL.h>
#include "engine.hpp"

class Hunko : public PlayerController {
 public:
  Hunko();
  void testCharacterInclude() override;

 private:
  void initializeAttacks() override;
  void initializeCharacterAttrs() override;
  void handleAttack(PlayerEntity* p, const ButtonStates* in) override;
};
