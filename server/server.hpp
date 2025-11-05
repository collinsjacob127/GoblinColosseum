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
#define ENABLE_AWAITING_NEW_PACKETS_NOTIF true

// constexpr int SERVER_PORT = 0;
constexpr char* SERVER_PORT = (char*)"53243";
constexpr ssize_t MAX_USERNAME_SIZE = 25;
constexpr ssize_t CLIENT_PACKET_N_BYTES = 42;
constexpr ssize_t CLIENT_CONTENTS_SIZE = 25;
constexpr ssize_t SERVER_CONTENTS_SIZE = 25;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 64;

class MatchmakingPacket {
 public:
  ssize_t pkt_size;
  ssize_t contents_size;

  uint8_t packet_type = 0;
  // Random number assigned by server. Used to self-identify when making requests.
  uint64_t session_id = 0;
  // Lobby number assigned by server.
  uint64_t lobby_id = 0;

  char contents[BUFFER_SIZE];

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

  PlayerEntry() {}
  PlayerEntry(int sock_desc, sockaddr_in addr) {
    // Set address info for new client
    socket_descriptor = sock_desc;
    address = addr;

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ipv4_str = std::string(ip_buffer);

    port_num = ntohs(address.sin_port);
  }

  void setNetInfo(int sock_desc, sockaddr_in addr) {
    // Set address info for new client
    socket_descriptor = sock_desc;
    address = addr;

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ipv4_str = std::string(ip_buffer);

    port_num = ntohs(address.sin_port);
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
 * @brief Function to populate a buffer with `n_bytes` bytes of data
 * from socket `s`.
 */
ClientPacket recvClientPacket(ssize_t n_bytes, int s);

/**
 * @brief Close every socket in a file descriptor set.
 */
void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd);

/**
 * @brief Given a username, return the corresponding registry entry.
 */
PlayerEntry getEntryFromUserName(std::string uname);

/**
 * @brief Player uname requests the list of open lobbies.
 * If they are marked as open_lobby, get rid of that 'cause they shouldn't be.
 * Don't return themselves.
 */
std::vector<std::string> getLobbyList();

/**
 * @brief Function to remove a player from the registry.
 * @note This does not disconnect a client from the server.
 */
void removePlayer(int fd);

/**
 * @brief Function to disconnect a client from the server.
 * @note This does NOT remove a player from the registry.
 */
void disconnectClient(int fd);

/**
 * @brief Function to send a packet to given client with contents "GOOD" or "BAD".
 * @return returns bytes sent or -1 on error.
 */
ssize_t sendConfirmationResponse(int fd, bool is_good);

void handleSigint(int signal_num);

/**
 * NET UTILS - courtesy of l'beej
 */

void packi64(unsigned char *buf, uint64_t i);

/**
 * @brief unpack a 64-bit unsigned from a char buffer (like ntohl())
 */
uint64_t unpacku64(unsigned char *buf);