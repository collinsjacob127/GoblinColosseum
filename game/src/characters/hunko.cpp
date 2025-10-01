/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "hunko.hpp"

Hunko::Hunko() {
  initializeCharacterAttrs();
  initializeAttacks();
  default_hurtboxes.push_back({0, 0, 100, 300});
}

void Hunko::initializeAttacks() {
  // Grounded attacks
  Attack illegal_homerun( "ILLEGAL_HOMERUN", B4, {DOWN, DOWN_RIGHT, RIGHT}, 30, 20, 60, 20);
  for (int i = 0; i <= 100; ++i) {
    illegal_homerun.hitbox_sets.push_back({ {0, 0, 30, 30} });
    illegal_homerun.hurtbox_sets.push_back({ {0, 0, 30, 30} });
  }
  illegal_homerun.state = GROUND_SPECIAL;
  gnd_specials.push_back(illegal_homerun);
}

void Hunko::initializeCharacterAttrs() {

}

void Hunko::handleAttack(PlayerEntity* p, const ButtonStates* in) {
  printPlayerFrames(p);
  if (p->f_recovery) {
    p->f_recovery--;
  }
  if (p->f_active) {
    if (--p->f_active == 0) { p->f_recovery = p->cur_attack->f_recovery; }
  }
  if (p->f_startup) {
    if (--p->f_startup == 0) { p->f_active = p->cur_attack->f_active; }
  }

  if (isActionable(p)) { 
    p->hitboxes = nullptr;
    p->hurtboxes = &default_hurtboxes;
    stand(p, in); 
    return; 
  }

  switch (p->state) {
    case (GROUND_NORMAL): { 
      std::cout << "Handling ground normal\n";
      break; 
    }
    case (GROUND_SPECIAL): { 
      if (p->cur_attack->name == "ILLEGAL_HOMERUN") {
        p->hitboxes = gnd_specials.at(0).getHitboxes(p->f_startup, p->f_active, p->f_recovery);
        p->hurtboxes = gnd_specials.at(0).getHurtboxes(p->f_startup, p->f_active, p->f_recovery);
      }
      break; 
    }
    case (AIR_NORMAL): { 
      std::cout << "Handling air normal\n";
      break; 
    }
    case (AIR_SPECIAL): { 
      std::cout << "Handling air special\n";
      break; 
    }
    default: { std::cerr << "Attack handled with invalid state\n"; }
  }
}

void Hunko::testCharacterInclude() {
  std::cout << "HUNKO IS HERE!" << std::endl;
}
