/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "hunko.hpp"

void Hunko::initializeAttacks() {
  // Grounded attacks
  Attack illegal_homerun( "ILLEGAL_HOMERUN", B4, {DOWN, DOWN_RIGHT, RIGHT}, 30, 5, 20, 5);
  gnd_specials.push_back(illegal_homerun);
}

void Hunko::initializeCharacterAttrs() {

}

void Hunko::testCharacterInclude() {
  std::cout << "HUNKO IS HERE!" << std::endl;
}
