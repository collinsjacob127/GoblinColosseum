/**
 * Author: Jacob Collins
 * 
 * Description:
 * Header for network functionality. 
 * Some networking is overloaded in net.cpp with platform-specific implementations.
 * Bro is the goat: https://beej.us/guide/bgnet/html/index-wide.html
 * 
 * High-level description:
 * - Each send/recv is done as a pair in a single connection instance.
 * - ClientPacket structs are sent to the server and ServerPacket structs are received.
 */

#pragma once

#include <iostream>
#include <string.h>
#include <sstream>
#include <memory>
#include <cstring>

#include "engine.hpp"
#include "util.hpp"

#define ENABLE_NETCODE_DEBUG true
#define ENABLE_NETCODE_ERROR true
#define ENABLE_NETCODE_LOG true
#define ENABLE_CLIENTPACKET_PRESEND_INSPECTION true


// IPV4 addr of the server
// constexpr char *SERVER_ADDR = (char*)"192.168.1.100";
constexpr char *SERVER_ADDR = (char*)"127.0.0.1";
constexpr int SERVER_PORT = 53243;

// Max SIZE of username
// Number of allowed characters in usernames is then MAX_USERNAME_SIZE-1
constexpr size_t MAX_USERNAME_SIZE = 25;
constexpr int BUFFER_SIZE = 1024;

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
 *            Server responds with usernames batched in 10s (0 - 9, X0 - X9).
 * 
 * @note 3 -> (JOIN) Send username of peer to join
 *            Server responds with lobby ID.
 * 
 * @note 4 -> (LOBBY <ID>) Send current lobby ID.
 *            Server responds with IP Info of connected peer, if any
 *            If no connection, responds with 0.0.0.0:0
 */
struct ClientPacket {
  uint8_t packet_type = 0;

  // Random number assigned by server. Used to self-identify when making requests.
  uint64_t session_id = 0;
  // Lobby number assigned by server.
  uint64_t lobby_id = 0;

  /**
   * Contents types (c-string):
   * @note 0 -> Own username
   * @note 1 -> "CREATE"
   * @note 2 -> "LIST"
   * @note 3 -> Peer username (?)
  */
  char contents[MAX_USERNAME_SIZE];

  /**
   * @brief Build a client packet given the type
   * and contents
   * @param type Header to inform server what type of message is being sent
   * @param val Contents of the message being sent
   * @note Possible values for type:
   * @note 0 -> Sending own username (Entering server). Expects server response "GOOD"
   * @note 1 -> (CREATE) Requesting to create a lobby. Expects server sesponse "GOOD"
   * @note 2 -> (LIST) Requesting list of available peers. 
   * @note 3 -> Requesting ip addr of peer. 
   */
  ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val);

  /**
   * @brief Build a ClientPacket from a received buffer.
   */
  ClientPacket(unsigned char* buf, ssize_t n_bytes);

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
 * @brief Struct describing structure of packets received from the server.
 */
struct ServerPacket {
  uint8_t type = 0;
  uint64_t session_id = 0;
  uint64_t lobby_id = 0;
  char contents[BUFFER_SIZE];
};

/**
 * @brief Driver class for all client-side network activity.
 */
class NetEngine {
 public:
  std::string username = "";
  uint64_t session_id = 0;
  uint64_t lobby_id = 0;
  int server_sock = -1;
  int peer_sock = -1;

  NetEngine();
  ~NetEngine();

  /**
   * @brief Function to get a user's name from cin.
   * @note Verifies that the name is valid and sets the username
   */
  void getLocalUserName();

  /**
   * @brief Function to set the value of the username
   * @note Verifies that the name is valid
   */
  void setLocalUserName(std::string user_name);

  /**
   * @brief Function to determine whether the user wishes to
   *        create a game or join an existing lobby
   * @return 0 for JOIN or 1 for CREATE
   */
  int getLocalJoinOrCreate();

  /**
   * @brief Function to test client-side network functionality
   * @note It is assumed that the username has already been set
   */
  int testNetClient();

  /**
   * @brief Function to inform the server of your username and get
   * a new session ID.
   */
  ssize_t initializeServerCommunication();

 private:

  /**
   * @brief Function to connect to the game's server
   */
  int serverConnect();

  /**
   * @brief Function to parse and send a ClientPacket to the server
   */
  ssize_t sendClientPacket(ClientPacket);

  /**
   * @brief Function to recv and parse a ServerPacket from the server
   */
  ServerPacket recvServerPacket();

  /**
   * @brief Function to disconnect from the server
   */
  void serverDisconnect();

  /**
   * @brief Function to send a user's username to the server
   * @note Verifies that the name is valid
   * @note If server sends back BAD, returns 1
   */
  int sendUserName();

  /**
   * @brief Function to send a CREATE message to the server
   */
  int sendCreate();

  /**
   * @brief Function to retrieve the current list of users from the server
   */
  std::vector<std::string> receiveServerList();

  /**
   * @brief Select which lobby to join via cin
   */
  size_t selectLobby(size_t n_lobbies);

  /**
   * @brief Send to the server the username of the peer you wish to join
   * @return 0 on success, -1 on failure
   */
  int sendJoinRequest(std::string peer_name);

  /**
   * @brief Read in the address of whatever peer you shall connect to for the game
   */
  std::pair<std::string, std::string> getPeerAddrInfo();

  /**
   * @brief Recieve from the server and parse the results
   * @return -1 on failure, 0 on success, 1 on server response != "GOOD"
   */
  ssize_t verifyGoodResponse();

  void peerDisconnect();
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