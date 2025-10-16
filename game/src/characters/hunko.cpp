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
  float default_y0 = default_hurtboxes[0].height/2;
  /*******************************
   ****** GROUNDED NORMALS *******
   *******************************/
  // BIG KICK!
  gnd_normals.push_back(Attack( "BIG_KICK", B3, {CENTER}, 30, 12, 24, 16));
  atk = &gnd_normals.at(gnd_normals.size()-1);
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  atk->id = 0;
  atk->damage = 60;
  atk->proration = 0.9;
  atk->x_vel = 10.0;
  atk->y_vel = 0.0;
  atk->level = 2.0;
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = default_x0;
    tmp_hitbox.y = default_y0 * 1.5;
    tmp_hitbox.width = 150;
    tmp_hitbox.height = 50;
    atk->hitbox_sets.push_back({tmp_hitbox});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }

  // Lil Punch
  gnd_normals.push_back(Attack( "LIL PUNCH", B1, {CENTER}, 30, 6, 3, 3));
  atk = &gnd_normals.at(gnd_normals.size()-1);
  atk->id = 0;
  atk->damage = 50;
  atk->proration = 0.7;
  atk->x_vel = 3.0;
  atk->y_vel = -0.5;
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = default_x0;
    tmp_hitbox.y = default_y0 / 2;
    tmp_hitbox.width = 80;
    tmp_hitbox.height = 25;
    atk->hitbox_sets.push_back({tmp_hitbox});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }

  // Set state for all grounded normals
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

  // Start CIVILLIAN SMASHER
  gnd_specials.push_back(Attack( "CIVILIAN_SMASHER", B3, {RIGHT, DOWN, DOWN_RIGHT}, 50, 12, 24, 16));
  // gnd_specials.push_back(Attack( "CIVILIAN_SMASHER", B3, {RIGHT, CENTER, RIGHT}, 15, 12, 24, 16));
  atk = &gnd_specials.at(gnd_specials.size()-1);
  atk->id = 0;
  atk->damage = 150;
  atk->proration = 0.8;
  atk->x_vel = 15.0;
  atk->y_vel = -10.0;
  atk->level = 4.0;
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
  // Start ILLEGAL HOMERUN
  gnd_specials.push_back(Attack( "ILLEGAL_HOMERUN", B4, {DOWN, DOWN_RIGHT, RIGHT}, 30, 10, 20, 8));
  atk = &gnd_specials.at(gnd_specials.size()-1);
  atk->id = 0;
  atk->damage = 80;
  atk->proration = 0.9;
  atk->x_vel = 5.0;
  atk->y_vel = -5.0;
  atk->level = 3.0;
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

  for (f = 0; f < gnd_specials.size(); ++f) {
    atk = &gnd_specials.at(f);
    atk->state = GROUND_SPECIAL;
  }
  /*******************************
   **** END GROUNDED SPECIAL *****
   *******************************/

  /*******************************
   ****** AERIAL  NORMALS *******
   *******************************/
  // Slam hammer!
  air_normals.push_back(Attack( "SLAM HAMMER", B3, {CENTER}, 30, 12, 24, 16));
  atk = &air_normals.at(air_normals.size()-1);
  atk->id = 0;
  atk->damage = 60;
  atk->proration = 0.7;
  atk->x_vel = 5.0;
  atk->y_vel = -5.0;
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = (default_x0) - (3 * f);
    tmp_hitbox.y = (default_y0 * 2);
    tmp_hitbox.width = 80;
    tmp_hitbox.height = 80;
    atk->hitbox_sets.push_back({tmp_hitbox});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define recovery boxes
  for (f = 0; f < atk->f_recovery; ++f) {
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }

  // Set state for all grounded normals
  for (f = 0; f < air_normals.size(); ++f) {
    atk = &air_normals.at(f);
    atk->state = AIR_NORMAL;
  }
  /*******************************
   **** END AERIAL  NORMALS *****
   *******************************/

  /*******************************
   ****** AERIAL  SPECIAL *******
   *******************************/

  // Start SKYBOX CRASHER
  air_specials.push_back(Attack( "SKYBOX_CRASHER", B4, {DOWN, CENTER, DOWN}, 30, 8, 30, 12));
  atk = &air_specials.at(air_specials.size()-1);
  atk->id = 0;
  atk->damage = 150;
  atk->proration = 0.8;
  atk->x_vel = 5.0;
  atk->y_vel = -8.0;
  atk->level = 4.0;
  // Define startup boxes
  for (f = 0; f < atk->f_startup; ++f) {
    // std::cout << "Pushing to atk hitbox sets\n";
    atk->hitbox_sets.push_back({});
    atk->hurtbox_sets.push_back(default_hurtboxes);
  }
  // Define active boxes
  for (f = 0; f < atk->f_active; ++f) {
    tmp_hitbox.x = -default_hurtboxes[0].width/2;
    tmp_hitbox.y = 2*default_y0;
    tmp_hitbox.width = 300;
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

  for (f = 0; f < air_specials.size(); ++f) {
    atk = &air_specials.at(f);
    atk->state = AIR_SPECIAL;
  }
  /*******************************
   **** END AERIAL  SPECIAL *****
   *******************************/
}

void Hunko::initializeCharacterAttrs() {

}

void Hunko::handleAttack(PlayerEntity* p, const ButtonStates* in) {
  if (p->f_recovery) {
    if (--p->f_recovery == 0) {
      // Exit the attack
      p->has_hit = false;
      // No current attack
      p->cur_attack = nullptr;
      
      // Reset hitboxes and hurtboxes to default
      // p->base_hitboxes = &default_hitboxes;
      // p->base_hurtboxes = &default_hurtboxes;
      updateBoxes(p);

      if (p->state == GROUND_NORMAL || p->state == GROUND_SPECIAL) {
        stand(p, in); 
      } else {
        fall(p, in);
      }
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
      // std::cout << "Grounded Normal (" << p->cur_attack->name << "): ";
      break;
    }
    case (GROUND_SPECIAL): { 
      // std::cout << "Grounded Special (" << p->cur_attack->name << "): ";
      break; 
    }
    case (AIR_NORMAL): { 
      handleAerial(p, in);
      // std::cout << "Aerial Normal (" << p->cur_attack->name << "): ";
      break; 
    }
    case (AIR_SPECIAL): { 
      handleAerial(p, in);
      // std::cout << "Aerial Special (" << p->cur_attack->name << "): ";
      break; 
    }
    default: { std::cerr << "Attack handled with invalid state\n"; }
  }

  // printPlayerFrames(p);
}

void Hunko::testCharacterInclude() {
  std::cout << "HUNKO IS HERE!" << std::endl;
}

int Hunko::getCharacterId() {
  return CHARACTER_ID_HUNKO;
}
