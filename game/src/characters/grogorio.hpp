/**
 * Author: Jacob Collins
 * Description:
 * This file has the PlayerController subclass for Grogorio, the Goblin Prince
 */

#pragma once

#include <iostream>
#include <iomanip>
#include <string.h>
#include <SDL3/SDL.h>
#include "engine.hpp"

class Grogorio : public PlayerController {
 public:
  Grogorio();
  void testCharacterInclude() override;

 private:
  int getCharacterId() override;
  void initializeAttacks() override;
  void initializeCharacterAttrs() override;
  void handleAttack(PlayerEntity* p, const ButtonStates* in) override;
};
