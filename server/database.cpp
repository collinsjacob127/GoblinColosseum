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
  if (ENABLE_REGISTRY_LOG) {
    std::stringstream ss;
    ss << "[Registry] Deleted " << n_removed << " entries from session map\n";
    ANSI_ESCAPES.printInColor(ss.str(), ANSI_ESCAPES.cyan_fg);
  }

  session_map.clear();
  player_map.clear();
  lobby_map.clear();
}

uint64_t Registry::addPlayer() {
  // Construct new player
  PlayerEntry* player = new PlayerEntry();
  // Generate ID and add player to session map
  uint64_t session_id = setNewId(player, SESSION_ID_SPECIFIER);

  return session_id;
}

int Registry::removePlayer(PlayerEntry* player) {
  // Verify valid ptr
  if (!player) { return -1; }

  // Get the session ID
  uint64_t *session_id = player->getIdOfType(SESSION_ID_SPECIFIER);
  if (!session_id) {
    if (ENABLE_REGISTRY_DEBUG) {
      std::cout << "[RegistryDBG] Attempted removal of player" 
      << " with no seesion ID" << std::endl;
    }
    delete player;
    return -1;
  }

  // Remove all player's map entries
  if (player->lobby_id && 
    getPlayer(player->lobby_id, LOBBY_ID_SPECIFIER)->session_id 
    == player->session_id) {

    clearId(player->lobby_id, LOBBY_ID_SPECIFIER);

  }

  if (player->player_id)
    clearId(player->player_id, PLAYER_ID_SPECIFIER);
  
  TYPE_PLAYER_MAP::iterator it = session_map.find(player->session_id);
  session_map.erase(it);
  
  delete player;
  return 1;
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
    std::stringstream ss;
    ss << "[Registry] " << getIdTypeName(id_type) << ": " 
    << rand_n << " provided to player: " << player->user_name << std::endl;
    ANSI_ESCAPES.printInColor(ss.str(), ANSI_ESCAPES.cyan_fg);
  }

  return rand_n;
}

void Registry::clearId(uint64_t id, TYPE_ID_SPECIFIER id_type) {
  // Get corresponding map
  TYPE_PLAYER_MAP *map = getMapOfType(id_type);
  if (!map) { return; }

  // Get corresponding player ptr
  PlayerEntry* player = getPlayer(id, id_type);
  if (!player) { return; }

  // If passed session ID remove the player entirely
  if (id_type == SESSION_ID_SPECIFIER) {
    removePlayer(player);  
  }

  // Clear player's ID
  uint64_t *id_ptr = player->getIdOfType(id_type); 
  if (!id_ptr) { return; } // invalid type
  *id_ptr = 0;

  // Get player's position in map
  TYPE_PLAYER_MAP::iterator it = map->find(id);
  if (it == map->end()) { return; } // Player not found

  if (ENABLE_REGISTRY_DEBUG) {
    std::stringstream ss;
    ss << "[RegistryDBG] Map (" << getIdTypeName(id_type) << ") size pre-clearId(): " 
    << map->size() << std::endl;
    ANSI_ESCAPES.printInColor(ss.str(), ANSI_ESCAPES.cyan_fg);
  }

  // Erase player from this map
  map->erase(it);

  if (ENABLE_REGISTRY_DEBUG) {
    std::stringstream ss;
    ss << "[RegistryDBG] Map (" << getIdTypeName(id_type) << ") size post-clearId(): " 
    << map->size() << std::endl;
    ANSI_ESCAPES.printInColor(ss.str(), ANSI_ESCAPES.cyan_fg);
  }
}

std::vector<TYPE_LOBBY_INFO> Registry::getLobbyList(size_t min_idx, size_t max_idx) {
  TYPE_PLAYER_MAP::iterator it;

  std::vector<TYPE_LOBBY_INFO> lobby_list;

  size_t cur_idx = 0;
  for (it = lobby_map.begin(); it != lobby_map.end(); it++) {
    // Only select in range
    if (cur_idx < min_idx || cur_idx >= max_idx) {
      cur_idx++;
      continue;
    }
    cur_idx++;

    // Var to store current lobby info
    TYPE_LOBBY_INFO cur_lobby("", 0);

    // Get this player
    PlayerEntry* player = (PlayerEntry*)&(*it->second);

    // Make sure ptr is good
    if (!player) { continue; }

    // Make sure lobby open
    if (player->match_made) { continue; }

    // Valid player, populate and add to list
    cur_lobby.first = player->user_name;
    cur_lobby.second = player->lobby_id;
    lobby_list.push_back(cur_lobby);
  }

  return lobby_list;
}

size_t Registry::getNumLobbies() { 
  size_t n_lobbies = 0;
  // Loop through lobby map
  for (TYPE_PLAYER_MAP::iterator it = lobby_map.begin(); it != lobby_map.end(); it++) {
    // Get lobby owner
    PlayerEntry* player = it->second;
    // Verify good ptr
    if (!player) { continue; }
    // Verify lobby open
    if (player->match_made) { continue; }
    // Increment open lobby count
    n_lobbies++;
  }
  // Return lobby count
  return n_lobbies; 
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

  // Iterate through the map
  for (it = map->begin(); it != map->end(); ++it) {
    // Make sure ptr in value mapping is not null
    if (!it->second) { continue; }

    // Get a pointer to the mapped player
    PlayerEntry* tmp = &(*it->second);

    // Make doubly sure pointer is valid
    if (tmp) {
      // Free PlayerEntry
      delete tmp;
      // Increment number of removals
      n_removed++;
    }
  }

  return n_removed;
}

std::string Registry::getIdTypeName(TYPE_ID_SPECIFIER id_type) {
  switch (id_type) {
    case (PLAYER_ID_SPECIFIER): { return "PLAYER_ID"; }
    case (LOBBY_ID_SPECIFIER): { return "LOBBY_ID"; }
    case (SESSION_ID_SPECIFIER): { return "SESSION_ID"; }
    default: { return "INVALID_ID"; }
  }
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
