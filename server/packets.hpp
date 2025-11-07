/**
 * Author: Jacob Collins
 * Date: 11/5/2025
 * Description: Header file for packet classes.
 */

#pragma once

#include <iostream> // output
#include <string.h> // strings
#include <sstream> // string streams
#include <vector>
#include <memory> // sized types

#define ENABLE_CLIENTPACKET_INSPECTION true


constexpr ssize_t CLIENT_CONTENTS_SIZE = 25;
constexpr ssize_t CLIENT_PACKET_N_BYTES = 42;

constexpr ssize_t SERVER_CONTENTS_SIZE = 1024;
constexpr ssize_t SERVER_PACKET_N_BYTES = (CLIENT_PACKET_N_BYTES - CLIENT_CONTENTS_SIZE) + SERVER_CONTENTS_SIZE;
constexpr size_t MAX_N_REQUESTED_LOBBIES = (SERVER_CONTENTS_SIZE / CLIENT_CONTENTS_SIZE) / 2;

constexpr int BUFFER_SIZE = SERVER_PACKET_N_BYTES;

typedef std::pair<std::string, uint64_t> TYPE_LOBBY_INFO;

class MatchmakingPacket {
 public:
  ssize_t pkt_size;
  ssize_t contents_size;

  uint8_t packet_type = 0;
  // Random number assigned by server. Used to self-identify when making requests.
  uint64_t session_id = 0;
  // Lobby number assigned by server.
  uint64_t lobby_id = 0;

  char contents[BUFFER_SIZE] = "";

  /**
   * @brief Function to move ClientPacket into a provided
   * buffer and prepare the contents for network send
   */
  ssize_t buildPacket(unsigned char* buf);

  /**
   * @brief Function to get a string representing the contents of a
   * buffer presumably filled by this struct's method
   */
  std::string getStringFromBuffer(unsigned char* buf, ssize_t n_bytes);

  /**
   * @brief Function to get a string representing the contents
   * of this struct.
   */
  std::string getStringFromSelf();

};

/**
 * @brief Struct for packaging packets sent to server
 * @note [ [Packet type - 1B] [Session ID - 8B] [Lobby ID - 8B] [Contents - 25B] ] Total - 44B
 * @note Packet types:
 * @note 0 -> Sending own username (Entering server). 
 *            Server responds with unique session ID
 * 
 * @note 1 -> (CREATE) Requesting to create a lobby. 
 *            Server responds with lobby ID.
 * 
 * @note 2 -> (LIST <X>) Requesting list of available peers. 
 *            sid replaced with min_lobby_idx
 *            lid replaced with max_lobby_idx
 *            Server responds with usernames batched in 10s (0 - 9, X0 - X9).
 * 
 * @note 3 -> (JOIN) Send username of peer to join
 *            Server responds with lobby ID.
 * 
 * @note 4 -> (LOBBY <ID>) Send current lobby ID.
 *            Server responds with IP Info of connected peer, if any
 *            If no connection, responds with 0.0.0.0:0
 */
class ClientPacket : public MatchmakingPacket {
 public:

  /**
   * @brief Build a client packet given the type
   * and contents
   */
  ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val);

  /**
   * @brief Build a ClientPacket from a received buffer.
   */
  ClientPacket(unsigned char* buf, ssize_t n_bytes);
};

class ServerPacket : public MatchmakingPacket {
 public:

  ServerPacket(uint8_t type, uint64_t sid, uint64_t lid, const char* val);

  ServerPacket(unsigned char* buf, ssize_t n_bytes);

  /**
   * @brief Function to populate the contents buffer with
   * @param type packet type
   * @param lobbies list of lobby username / id pairs
   */
  ServerPacket(uint8_t type, uint64_t n_sent, uint64_t n_total, std::vector<TYPE_LOBBY_INFO> lobby_list);

  /**
   * @brief Function to parse a received lobby list
   * @return The lobby list. Returns empty vector on error.
   */
  std::vector<TYPE_LOBBY_INFO> parseLobbyList();
};


/**
 * Net UTILS (Provided by beej - https://beej.us/guide/bgnet/html/index-wide.html#sonofdataencap)
 */

/**
 * @brief store a 64-bit int into a char buffer (like htonl())
 */
void packi64(unsigned char *buf, uint64_t i);

/**
 * @brief unpack a 64-bit unsigned from a char buffer (like ntohl())
 */
uint64_t unpacku64(unsigned char *buf);
