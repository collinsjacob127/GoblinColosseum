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
#define ENABLE_PACKET_INSPECTION true


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
  std::vector<std::string> lobby_list;

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
   * @return Bytes sent, or -1 on failure.
   */
  ssize_t initializeServerCommunication();

  /**
   * @brief Function to request the creation of a lobby
   * @return Bytes sent, or -1 on failure
   */
  ssize_t createLobby();

  /**
   * @brief Function to get a list of open lobbies from the server.
   * @return Bytes sent, or -1 on failure
   */
  ssize_t getLobbies(size_t min_idx, size_t max_idx);

 private:

  /**
   * @brief Function to connect to the game's server
   */
  int serverConnect();

  /**
   * @brief Function to parse and send a ClientPacket to the server
   * @brief Returns bytes sent or -1 on error
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
   * @brief Function to disconnect from a peer
   */
  void peerDisconnect();
};
