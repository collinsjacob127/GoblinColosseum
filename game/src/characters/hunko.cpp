/**
 * Definitions for engine functions.
 * Keybinds, actions, movement
 */

#include "hunko.hpp"

Hunko::Hunko() {
  // Define temporary box for saving in defaults
  BoxEntity tmp_hurtbox;
  tmp_hurtbox.x = -75;
  tmp_hurtbox.y = 0;
  tmp_hurtbox.width = 150;
  tmp_hurtbox.height = 400;
  default_hurtboxes.push_back(tmp_hurtbox);

  // BoxEntity tmp_hitbox;
  // tmp_hitbox.x = 50;
  // tmp_hitbox.y = 0;
  // tmp_hitbox.width = 50;
  // tmp_hitbox.height = 50;
  // default_hitboxes.push_back(tmp_hitbox);
  default_hitboxes = std::vector<BoxEntity>();

  initializeCharacterAttrs();
  initializeAttacks();
}

void Hunko::initializeAttacks() {
  // HITBOX AND HURTBOX COORDINATES ARE RELATIVE TO PLAYER'S CENTER

  Attack* atk; // Pointer to whatever attack is being defined
  size_t f = 0; // Used for indexing frames
  BoxEntity tmp_hitbox; // Used to define hitboxes
  float default_x0 = -default_hurtboxes[0].x;
  // float default_x0 = 0;
  float default_y0 = default_hurtboxes[0].y/2;
  /*******************************
   ****** GROUNDED NORMALS *******
   *******************************/
  gnd_normals.push_back(Attack( "CIVILIAN_SMASHER", B3, {CENTER, RIGHT, DOWN, DOWN_RIGHT}, 30, 12, 24, 16));
  atk = &gnd_normals.at(gnd_normals.size()-1);
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = default_x0 + 5 * f;
    tmp_hitbox.width = default_y0 + 10 * f;
    tmp_hitbox.height = 150;
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({tmp_hitbox});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  for (f = 0; f < gnd_normals.size(); ++f) {
    atk = &gnd_normals.at(f);
    atk->state = GROUND_NORMAL;
  }
  /*******************************
   **** END GROUNDED NORMALS *****
   *******************************/

  /*******************************
   ****** GROUNDED SPECIAL *******
   *******************************/
  // Start ILLEGAL HOMERUN
  gnd_specials.push_back(Attack( "ILLEGAL_HOMERUN", B4, {DOWN, DOWN_RIGHT, RIGHT}, 30, 10, 20, 8));
  atk = &gnd_specials.at(gnd_specials.size()-1);
  // Set hitboxes etc. for ILLEGAL HOMERUN
  tmp_hitbox.x = default_x0;
  tmp_hitbox.y = 0;
  tmp_hitbox.width = 200;
  tmp_hitbox.height = 200;
  BoxEntity tmp2;
  tmp2.x = default_x0 + 200;
  tmp2.y = 50;
  tmp2.width = 100;
  tmp2.height = 100;
  // Define startup
  for (f = 0; f < atk->f_startup; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    atk->hitbox_sets.push_back({tmp_hitbox, tmp2});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }

  // END ILLEGAL HOMERUN

  // Start CIVILLIAN SMASHER
  gnd_specials.push_back(Attack( "CIVILIAN_SMASHER", B3, {CENTER, RIGHT, DOWN, DOWN_RIGHT}, 30, 12, 24, 16));
  atk = &gnd_specials.at(gnd_specials.size()-1);
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = default_x0 + 5 * f;
    tmp_hitbox.width = 300 + 5 * f;
    tmp_hitbox.height = 150;
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({tmp_hitbox});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // END CIVILLIAN SMASHER

  for (f = 0; f < gnd_specials.size(); ++f) {
    atk = &gnd_specials.at(f);
    atk->state = GROUND_SPECIAL;
  }
  /*******************************
   **** END GROUNDED SPECIAL *****
   *******************************/
}

void Hunko::initializeCharacterAttrs() {

}

void Hunko::handleAttack(PlayerEntity* p, const ButtonStates* in) {
  printPlayerFrames(p);
  if (p->f_recovery) {
    if (--p->f_recovery == 0) {
      // Exit the attack
      // No current attack
      p->cur_attack = nullptr;
      
      // Reset hitboxes and hurtboxes to default
      p->base_hitboxes = &default_hitboxes;
      p->base_hurtboxes = &default_hurtboxes;
      stand(p, in); 
      return; 
    }
  }
  if (p->f_active) {
    if (--p->f_active == 0) { p->f_recovery = p->cur_attack->f_recovery; }
  }
  if (p->f_startup) {
    if (--p->f_startup == 0) { p->f_active = p->cur_attack->f_active; }
  }

  switch (p->state) {
    case (GROUND_NORMAL): { 
      std::cout << "Handling ground normal\n";
      break; 
    }
    case (GROUND_SPECIAL): { 
      // if (p->cur_attack->name == "ILLEGAL_HOMERUN") {
      //   p->hitboxes = gnd_specials.at(0).getHitboxes(p->f_startup, p->f_active, p->f_recovery);
      //   p->hurtboxes = gnd_specials.at(0).getHurtboxes(p->f_startup, p->f_active, p->f_recovery);
      // }
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
