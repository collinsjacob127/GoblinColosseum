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
#include "packets.hpp"

#define ENABLE_NETCODE_DEBUG true
#define ENABLE_NETCODE_ERROR true
#define ENABLE_NETCODE_LOG true


// IPV4 addr of the server
// constexpr char *SERVER_ADDR = (char*)"192.168.1.100";
constexpr char *SERVER_ADDR = (char*)"127.0.0.1";
constexpr int SERVER_PORT = 53243;

// Max SIZE of username
// Number of allowed characters in usernames is then MAX_USERNAME_SIZE-1
constexpr size_t MAX_USERNAME_SIZE = 25;

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
   * @param s socket descriptor
   * @return Valid packet on success, packet with all 0s and empty string on failure.
   */
  ServerPacket recvServerPacket(int s);

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