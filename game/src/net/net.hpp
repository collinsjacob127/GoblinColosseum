/**
 * Headers for functions relating to rendering which interact with main.cpp
 */

#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <cstring>

#include "engine.hpp"
#include "util.hpp"

#define ENABLE_NETCODE_DEBUG true

constexpr char *SERVER_ADDR = (char*)"192.168.1.100";
constexpr size_t MAX_USERNAME_LEN = 24;

struct ClientServerConnectPacket {
  
};

struct P2PConnectInfo {
  std::string peer_addr = "";
  int port = 0;
  int character_id = CHARACTER_ID_HUNKO;
};

class NetEngine {
 public:
  std::string username;
  int server_sock;
  int peer_sock;

  NetEngine();
  ~NetEngine();

  /**
   * @brief Function to get a user's name from cin.
   * @note Verifies that the name is valid
   */
  void getUserName();

  /**
   * @brief Function to test client-side network functionality
   */
  int testNetClient();

 private:

  /**
   * @brief Function to connect to the game's server
   */
  void connectToServer();

  /**
   * @brief Function to retrieve the current list of users from the server
   */
  std::vector<std::string> recieveServerList();

  /**
   * @brief Function to determine whether the user wishes to
   *        create a game or join an existing lobby
   * @return 0 for JOIN or 1 for CREATE
   */
  int getJoinOrCreate();

  /**
   * @brief Select which lobby to join via cin
   */
  int getJoinInfo(size_t n_lobbies);

  /**
   * @brief Read in the address of whatever peer you shall connect to for the game
   */
  std::pair<std::string, std::string> getPeerAddrInfo();

  void disconnectServer();
  void disconnectPeer();
};
