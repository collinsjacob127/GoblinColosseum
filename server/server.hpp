/**
 * Author: Jacob Collins
 * Date: 11/4/2025
 * Description:
 * Headers for the Goblin Colosseum matchmaking server
 */

#pragma once

#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <map>
#include <csignal> // Handle SIGINT
#include <memory>
#include <cstring>
#include <netdb.h>
// #include <sys/poll.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.hpp"


#define ENABLE_SERVER_LOG true
#define ENABLE_SERVER_DEBUG true
#define ENABLE_SERVER_ERROR true
#define ENABLE_CLIENTPACKET_INSPECTION true

// constexpr int SERVER_PORT = 0;
constexpr char* SERVER_PORT = (char*)"53243";
constexpr ssize_t MAX_USERNAME_SIZE = 25;
constexpr ssize_t MAX_PACKET_SIZE = 26;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 64;


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

struct PlayerEntry {
  uint64_t id = 0;            // Unique ID for this peer
  std::string user_name = "";
  bool match_made = false;
  bool open_lobby = false;
  int socket_descriptor = 0;
  struct sockaddr_in address; // IP addr & port num
  std::string ipv4_str = "";
  uint16_t port_num;
  Timer p_timer;
  Timer lobby_update_time; 

  PlayerEntry() { p_timer.start(); }
  PlayerEntry(int sock_desc, sockaddr_in addr) {
    // Set address info for new client
    socket_descriptor = sock_desc;
    address = addr;

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ipv4_str = std::string(ip_buffer);

    port_num = ntohs(address.sin_port);

    // Start their timer
    p_timer.start();
  }

  std::string getReprString() {
    std::stringstream ss;

    // Set ipv4 str if not yet set
    if (ipv4_str == "") {
      char ip_buffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
      ipv4_str = std::string(ip_buffer);
    }

    ss << "ID: " << id;
    ss << " Sock Desc: " << socket_descriptor;
    ss << " IP: " << ipv4_str << ":" << ntohs(address.sin_port);
    return ss.str();
  }
};

/**
 * @brief Create, bind and passive open a socket on a local interface for the provided
 * service. Argument matches the second argument to getaddrinfo(3).
 *
 * @return Passively opened socket or -1 on error. Caller is responsible for
 * calling accept and closing the socket.
 */
int bindAndListen(const char *service);

/**
 * @brief Close every socket in a file descriptor set.
 */
void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd);

/**
 * @brief Given a username, return the corresponding registry entry.
 */
PlayerEntry getEntryFromUserName(std::map<int, PlayerEntry> *registry, std::string uname);

/**
 * @brief Player uname requests the list of open lobbies.
 * If they are marked as open_lobby, get rid of that 'cause they shouldn't be.
 * Don't return themselves.
 */
std::vector<std::string> getLobbyList(std::map<int, PlayerEntry> *registry);

/**
 * @brief Function to disconnect a client from the registry etc.
 */
void disconnectPlayer(std::map<int, PlayerEntry> *registry, int fd, fd_set *sock_set);

/**
 * @brief Function to send a packet to given client with contents "GOOD" or "BAD".
 * @return returns bytes sent or -1 on error.
 */
ssize_t sendConfirmationResponse(int fd, bool is_good);

void handleSigint(int signal_num);


