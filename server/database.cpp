/**
 * Author: Jacob Collins
 * Date: 11/5/2025
 * Description:
 * Definitions for server data structures
 * GoblinEntry: Class containing permanant player information.
 * PlayerEntry: Struct containing instanced player information.
 * Registry: Instanced mapping of session IDs to PlayerEntities for matchmaking.
 * PlayerMap: Persistent saved mapping of unique player IDs to PlayerEntities.
 * LeaderBoard: History of wins/losses by Player and Character.
 */

#include "database.hpp"

/**********************
 * PLAYER ENTRY START
 *********************/

PlayerEntry::PlayerEntry(uint64_t session_id, std::string u_name) {
  session_id = session_id;
  user_name = u_name;
}

std::string PlayerEntry::getReprString() {
  std::stringstream ss;
  ss << std::endl;
  ss << "  [Player] uname: " << user_name << std::endl;
  ss << "  [Player] s_id: " << session_id << std::endl;
  ss << "  [Player] p_id: " << player_id << std::endl;
  ss << "  [Player] l_id: " << lobby_id << std::endl;
  return ss.str();
}

uint64_t* PlayerEntry::getIdOfType(TYPE_ID_SPECIFIER id_type) {
  switch (id_type) {
    case (SESSION_ID_SPECIFIER):
      return &session_id;
    case (PLAYER_ID_SPECIFIER):
      return &player_id;
    case (LOBBY_ID_SPECIFIER):
      return &lobby_id;
    default:
      return nullptr;
  }
}

/**********************
 * PLAYER ENTRY FINISH
 *********************/


/**********************
 * REGISTRY START
 *********************/

Registry::Registry() {

}

Registry::~Registry() {
  ssize_t n_removed;

  n_removed = clearMap(&session_map);
  if (ENABLE_REGISTRY_LOG)
    std::cout << "[Registry] Deleted " << n_removed << " entries from session map\n";

  n_removed = clearMap(&player_map);
  if (ENABLE_REGISTRY_LOG)
    std::cout << "[Registry] Deleted " << n_removed << " entries from player map\n";

  n_removed = clearMap(&lobby_map);
  if (ENABLE_REGISTRY_LOG)
    std::cout << "[Registry] Deleted " << n_removed << " entries from lobby map\n";
}

uint64_t Registry::addPlayer() {
  // Construct new player
  PlayerEntry* player = new PlayerEntry();
  // Generate ID and add player to session map
  uint64_t session_id = setNewId(player, SESSION_ID_SPECIFIER);

  return session_id;
}

PlayerEntry* Registry::getPlayer(uint64_t id, TYPE_ID_SPECIFIER id_type) {
  // Get corresponding map
  TYPE_PLAYER_MAP *map = getMapOfType(id_type);
  if (!map) { return nullptr; }

  // Check if player exists with this ID
  if (map->find(id) == map->end()) {
    // Doesn't exist
    return nullptr;
  }

  // Player exists, return ptr
  return map->at(id);
}

uint64_t Registry::setNewId(PlayerEntry* player, TYPE_ID_SPECIFIER id_type) {
  // Get map corresponding to requested ID type
  TYPE_PLAYER_MAP *map = getMapOfType(id_type);
  if (!map) { return 0; }

  // Get the corresponding ID already in player to verify unset
  uint64_t *id = player->getIdOfType(id_type);
  if (!id) { return 0; } // Bad type
  if (*id != 0) {
    // ID already set
    return 0;
  }

  // Generate new ID
  uint64_t rand_n = id_generator.getRandomId();
  
  // Guarantee unique session ID
  while (map->find(rand_n) != map->end()) {
    rand_n = id_generator.getRandomId();
  }

  //Set ID for player
  *id = rand_n;

  // Insert player to map
  map->insert_or_assign(rand_n, player);

  if (ENABLE_REGISTRY_LOG) {
    std::cout << "[Registry] ID " << rand_n << " (type " << id_type << ") "
    << "provided to player: " << player->getReprString();
  }

  return rand_n;
}

void Registry::clearId(uint64_t id, TYPE_ID_SPECIFIER id_type) {
  // Get corresponding map
  TYPE_PLAYER_MAP *map = getMapOfType(id_type);
  if (!map) { return; }

  // Get corresponding player ptr
  PlayerEntry* player = getPlayer(id, id_type);

  // Clear player's ID
  uint64_t *id_ptr = player->getIdOfType(id_type); 
  if (!id_ptr) { return; } // invalid type
  *id_ptr = 0;

  // Get player's position in map
  TYPE_PLAYER_MAP::iterator it = map->find(id);
  if (it == map->end()) { return; } // Player not found

  if (ENABLE_REGISTRY_DEBUG) {
    std::cout << "[RegistryDBG] Map (" << id_type << ") size pre-clearId(): " 
    << map->size() << std::endl;
  }

  // Erase player from this map
  map->erase(it);

  if (ENABLE_REGISTRY_DEBUG) {
    std::cout << "[RegistryDBG] Map (" << id_type << ") size post-clearId(): " 
    << map->size() << std::endl;
  }
}

size_t Registry::size() {
  return session_map.size();
}

TYPE_PLAYER_MAP* Registry::getMapOfType(TYPE_ID_SPECIFIER id_type) {
  switch (id_type) {
    case (SESSION_ID_SPECIFIER):
      return &session_map;
    case (PLAYER_ID_SPECIFIER):
      return &player_map;
    case (LOBBY_ID_SPECIFIER):
      return &lobby_map;
    default:
      return nullptr;
  }
}

ssize_t Registry::clearMap(TYPE_PLAYER_MAP* map) {
  ssize_t n_removed = 0;
  TYPE_PLAYER_MAP::iterator it;
  for (it = map->begin(); it != map->end(); it++) {
    if ((PlayerEntry*)it->second != nullptr) {
      free(it->second);
      n_removed++;
    }
  }
  return n_removed;
}

/**********************
 * REGISTRY FINISH
 *********************/


/**********************
 * PLAYER MAP START
 *********************/

/**********************
 * PLAYER MAP FINISH
 *********************/
