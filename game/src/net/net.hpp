/**
 * Header for network functionality. 
 * Overloaded in header.cpp with platform-specific implementations.
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

// IPV4 addr of the server
constexpr char *SERVER_ADDR = (char*)"192.168.1.100";

// Number of allowed characters in usernames
// Max SIZE of usernames is then MAX_USERNAME_LEN + 1
constexpr size_t MAX_USERNAME_LEN = 24;

/**
 * @brief Struct for packaging packets sent to server
 */
struct ClientPacket {
  /* Packet types:
  0 -> sending own username 
  1 -> declaring join type (JOIN / CREATE)
  2 -> sending join request username
  */
  char packet_type = 0;

  /* Contents types (c-string):
  packet_type = 0 -> "Example_Username"
  packet_type = 1 -> "JOIN" or "CREATE"
  packet_type = 2 -> "Example_Peer_Name"
  */
  char contents[MAX_USERNAME_LEN+1];
};

struct P2PConnectInfo {
  unsigned char ipv4_addr[4];
  int16_t port = 0;
  char character_id = CHARACTER_ID_HUNKO;
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
   * @note Verifies that the name is valid and sets the username
   */
  void getUserName();

  /**
   * @brief Function to set the value of the username
   * @note Verifies that the name is valid
   */
  void setUserName(std::string user_name);

  /**
   * @brief Function to test client-side network functionality
   * @note It is assumed that the username has already been set
   */
  int testNetClient();

 private:

  /**
   * @brief Function to connect to the game's server
   */
  int connectToServer();

  /**
   * @brief Function to send a user's username to the server
   * @note Verifies that the name is valid
   */
  int sendUserName();

  /**
   * @brief Function to send a JOIN message to the server
   */
  int sendJoin();

  /**
   * @brief Function to send a CONNECT message to the server
   */
  int sendConnect();

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
