/**
 * Header for network functionality. 
 * Overloaded in header.cpp with platform-specific implementations.
 * Bro is the goat: https://beej.us/guide/bgnet/html/index-wide.html
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
  * Struct for holding ipv4 and port values
 */
struct AddrInfoPkt {
  uint32_t ipv4_addr = 0;
  uint16_t port_num = 0; 

  // void setFromRaw(unsigned char *buf);
};

/**
 * @brief Struct for packaging packets sent to server
 */
struct ClientPacket {
  /* Packet types:
   * @note 0 -> Sending own username (Entering server). Expects server response "GOOD"
   * @note 1 -> (CREATE) Requesting to create a lobby. Expects server sesponse "GOOD"
   * @note 2 -> (LIST) Requesting list of available peers. 
   * @note 3 -> Requesting ip addr of peer. 
  */
  char packet_type = 0;

  /* Contents types (c-string):
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
  ClientPacket(char type, std::string val);

  /**
   * @brief Function to move ClientPacket into a provided
   * buffer and prepare the contents for network send
   */
  ssize_t sendPacket(int fd);
};

struct P2PConnectInfo {
  // unsigned char ipv4_addr[4];
  // int16_t port = 0;
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
   * @note If server sends back BAD, returns 1
   */
  int sendUserName();

  /**
   * @brief Function to determine whether the user wishes to
   *        create a game or join an existing lobby
   * @return 0 for JOIN or 1 for CREATE
   */
  int getJoinOrCreate();

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

  void disconnectServer();
  void disconnectPeer();
};
