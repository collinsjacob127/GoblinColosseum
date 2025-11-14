/**
 * Author: Jacob Collins
 * Date: 11/5/2025
 * Description: Header file for packet classes.
 */

#pragma once

#include <iostream> // output
#include <iomanip>  // output formatting
#include <string.h> // strings
#include <sstream> // string streams
#include <vector>
#include <memory> // sized types

#include "util.hpp"

#define ENABLE_CLIENTPACKET_INSPECTION false

constexpr ssize_t CLIENT_CONTENTS_SIZE = 25;
constexpr ssize_t CLIENT_PACKET_N_BYTES = 42;

constexpr ssize_t SERVER_CONTENTS_SIZE = 1024;
constexpr ssize_t SERVER_PACKET_N_BYTES = (CLIENT_PACKET_N_BYTES - CLIENT_CONTENTS_SIZE) + SERVER_CONTENTS_SIZE;
constexpr size_t MAX_N_REQUESTED_LOBBIES = (SERVER_CONTENTS_SIZE / CLIENT_CONTENTS_SIZE) / 2;

constexpr int BUFFER_SIZE = SERVER_PACKET_N_BYTES;

typedef std::pair<std::string, uint64_t> TYPE_LOBBY_INFO;

struct clientAddrInfo {
  uint32_t addr = 0;
  uint16_t port = 0;
  std::string rep_str = "0.0.0.0:0";

  clientAddrInfo(){}
  clientAddrInfo(uint32_t in_addr, uint16_t in_port);
  clientAddrInfo(std::string ipv4_str);
};

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
   * @brief Function to get a string representing the contents
   * of this struct.
   */
  std::string getStringFromSelf();

  /**
   * @brief Function to get a string representing the contents
   */
  std::string getStringFromBuffer(unsigned char* buf, ssize_t n_bytes);
};

/**
 * @brief Struct for packaging packets sent to server
 * @note [ [Packet type - 1B] [Session ID - 8B] [Lobby ID - 8B] [Contents - 25B] ] Total - 44B
 * @note Packet types:
 * 
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
 * @note 3 -> (JOIN) Send lobby ID to join (session, lobbyid, "")
 *            Server responds with lobby id on success, 0 on failure
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
   * @brief Function to populate the contents buffer with the open lobby list
   * @param type packet type
   * @param lobbies list of lobby username / id pairs
   * @note Populate the contents like so:
   * @note [lobby0_username] [lobby0_id] [lobby1username] [lobby1id]...
   * @note where all chunks are CLIENT_CONTENTS_SIZE bytes
   */
  ServerPacket(uint8_t type, uint64_t n_sent, uint64_t n_total, std::vector<TYPE_LOBBY_INFO> lobby_list);

  /**
   * @brief Function to populate the contents buffer with a peer's IP address & port #
   * @param peer_addr Address of the client's matchmade peer
   * @note Populate the contents like so:
   * @note [public ipv4 - 4 bytes, uint32_t] [public port - 2 bytes, uint16_t] [private ipv4 - 4 bytes, uint32_t] [private port - 2 bytes, uint16_t]...
   */
  ServerPacket(uint8_t type, uint64_t sid, uint64_t lid, clientAddrInfo peer_addr_pub, clientAddrInfo peer_addr_priv);

  /**
   * @brief Function to parse a received lobby list
   * @return The lobby list. Returns empty vector on error.
   */
  std::vector<TYPE_LOBBY_INFO> parseLobbyList();

  /**
   * @brief Function to parse a received lobby list
   * @return The lobby list. Returns empty vector on error.
   */
  clientAddrInfo parseAddrInfo();
};


/**
 * Net UTILS (Provided by beej - https://beej.us/guide/bgnet/html/index-wide.html#sonofdataencap)
 */

/**
 * @brief store a 16-bit int into a char buffer (like htonl())
 */
void packi16(unsigned char *buf, unsigned int i);

/**
 * @brief store a 32-bit int into a char buffer (like htonl())
 */
void packi32(unsigned char *buf, unsigned long int i);

/**
 * @brief store a 64-bit int into a char buffer (like htonl())
 */
void packi64(unsigned char *buf, uint64_t i);

/**
 * @brief unpack a 16-bit unsigned from a char buffer (like ntohs())
 */
uint16_t unpacku16(unsigned char *buf);

/**
 * @brief unpack a 32-bit unsigned from a char buffer (like ntohl())
 */
uint32_t unpacku32(unsigned char *buf);

/**
 * @brief unpack a 64-bit unsigned from a char buffer (like ntohl())
 */
uint64_t unpacku64(unsigned char *buf);
