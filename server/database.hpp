/**
 * Author: Jacob Collins
 * Date: 11/5/2025
 * Description:
 * Headers for server data structures.
 * Lobbies: Instanced mapping of session IDs to PlayerEntities for matchmaking.
 * Registry: Persistent saved mapping of unique player IDs to PlayerEntities.
 * LeaderBoard: History of wins/losses by Player and Character.
 */

#pragma once

#include <iostream>    // IO
#include <string.h>    // Strings
#include <memory>      // Sized types
#include <random>      // ID Generation
#include <map>         // Mapping IDs to players

#include "util.hpp"
#include "packets.hpp"

#define ENABLE_REGISTRY_LOG true
#define ENABLE_REGISTRY_DEBUG true

constexpr ssize_t MAX_USERNAME_SIZE = 25;

constexpr uint64_t MIN_ID_VALUE = 1'000'000'000ULL;
constexpr uint64_t MAX_ID_VALUE = std::numeric_limits<std::uint64_t>::max();

typedef int TYPE_ID_SPECIFIER;
constexpr TYPE_ID_SPECIFIER SESSION_ID_SPECIFIER = 0;
constexpr TYPE_ID_SPECIFIER PLAYER_ID_SPECIFIER = 1;
constexpr TYPE_ID_SPECIFIER LOBBY_ID_SPECIFIER = 2;

typedef std::pair<std::string, uint64_t> TYPE_LOBBY_INFO;

struct PlayerEntry {
  uint64_t session_id = 0;            // Unique ID for this player's session
  uint64_t player_id = 0;            // Unique ID for this player
  uint64_t lobby_id = 0;            // Unique ID for this player's lobby

  std::string user_name = "";
  Timer p_timer;              // Time of player join

  Timer lobby_update_time;    // Time of lobby create

  PlayerEntry() {}
  PlayerEntry(uint64_t session_id, std::string u_name);

  /**
   * @brief Helper to get a string with identifying information
   */
  std::string getReprString();

  /**
   * @brief Get a pointer to the corresponding ID
   */
  uint64_t* getIdOfType(TYPE_ID_SPECIFIER id_type);

};

/**
 * @brief Simple class to generate random values within
 * the valid ID range.
 */
class IdGenerator {
 public:
  std::random_device rd;
  std::mt19937_64 gen;
  std::uniform_int_distribution<uint64_t> dist;

  IdGenerator() {
    gen = std::mt19937_64(rd());
    dist = std::uniform_int_distribution<uint64_t>(MIN_ID_VALUE, MAX_ID_VALUE);
  }

  uint64_t getRandomId() {
    return dist(gen);
  }

};

// Map IDs to PlayerEntry pointers.
typedef std::map<uint64_t, PlayerEntry*> TYPE_PLAYER_MAP;

/**
 * @brief Primary data structure 
 */
class Registry {
 public:
  
  Registry();
  ~Registry();
  
  IdGenerator id_generator;

  /**
   * @brief Function to add a new player to the registry.
   * @return Generated session ID for the new player
   */
  uint64_t addPlayer();

  /**
   * @brief Function to remove a player entirely from the registry
   * @param player Pointer to the player to be deleted
   * @return 1 on success, -1 on failure
   * @warning THIS WILL FREE THE PLAYER'S POINTER
   */
  int removePlayer(PlayerEntry* player);

  /**
   * @brief Function to get pointer to player given one of their ids
   * @param id One of their Ids
   * @param id_type Type specifier for that ID
   * @return Pointer to the requested player. nullptr on failure.
   */
  PlayerEntry* getPlayer(uint64_t id, TYPE_ID_SPECIFIER id_type);

  /**
   * @brief Function to generate a new ID for this player
   * @param id_type specifier for which type of ID to set. 
   * @return The generated ID for the player. Returns 0 on failure.
   */
  uint64_t setNewId(PlayerEntry* player, TYPE_ID_SPECIFIER id_type);

  /**
   * @brief Function to reset a given ID type to 0 for a player
   * and remove the corresponding mapping.
   * @note If id type is SESSION_ID_SPECIFIER, deletes the player
   * from all mappings and removes them entirely.
   */
  void clearId(uint64_t id, TYPE_ID_SPECIFIER id_type);

  /**
   * @brief Get the list of currently open lobbies
   * @param min_idx The starting point of requested lobby list
   * @param max_idx The ending point of requested lobby list
   * @return A vector of TYPE_LOBBY_INFO. Each entry is 
   * a (username, lobby_id) pair.
   */
  std::vector<TYPE_LOBBY_INFO> getLobbyList(size_t min_idx, size_t max_idx);

  /**
   * @return The number of players connected.
   */
  size_t size();

 private:
  TYPE_PLAYER_MAP session_map;
  TYPE_PLAYER_MAP player_map;
  TYPE_PLAYER_MAP lobby_map;

  size_t n_entries;

  TYPE_PLAYER_MAP* getMapOfType(TYPE_ID_SPECIFIER id_type);

  /**
   * @brief Delete all entries in a map.
   * @return The number of entries deleted.
   */
  ssize_t clearMap(TYPE_PLAYER_MAP* map);
};
