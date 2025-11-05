/**
 * Author: Jacob Collins
 * Date: 11/4/2025
 * Description:
 * Headers for the Goblin Colosseum matchmaking server
 */

#pragma once

#include <iostream>
// #include <string>
#include <string.h>
#include <sstream>
#include <map>
#include <csignal> // Handle SIGINT
#include <memory>
#include <cstring>
#include <netdb.h>
#include <random>
// #include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.hpp"
#include "packets.hpp"


#define ENABLE_SERVER_LOG true
#define ENABLE_SERVER_DEBUG true
#define ENABLE_SERVER_ERROR true
#define ENABLE_AWAITING_NEW_PACKETS_NOTIF true

constexpr char* SERVER_PORT = (char*)"53243";
constexpr int MAX_PENDING = 64;

constexpr ssize_t MAX_USERNAME_SIZE = 25;

constexpr uint64_t MIN_ID_VALUE = 1'000'000'000ULL;
constexpr uint64_t MAX_ID_VALUE = std::numeric_limits<std::uint64_t>::max();


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
  PlayerEntry(uint64_t session_id, std::string u_name) {
    id = session_id;
    user_name = u_name;
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
 * BASIC SERVER FUNCTIONALITY
 */

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

ssize_t sendServerPacket(ServerPacket, int s);

/**
 * COMMUNICATION HANDLING
 */

int initializePlayer(ClientPacket in_pkt, int client_sock);

int createLobby(ClientPacket in_pkt, int client_sock);

int sendLobbies(ClientPacket in_pkt, int client_sock);

int sendPeerInfo(ClientPacket in_pkt, int client_sock);

/**
 * REGISTRY HELPERS
 */

/**
 * @brief Get a unique SESSION id 
 */
uint64_t generateSessionId();

/**
 * @brief Get a unique SESSION id 
 */
uint64_t generateLobbyId();

/**
 * @brief Given the current registry, return the PlayerEntry of whichever
 * client has the corresponding username.
 * @return Returns the corrent player entry, or default player entry on not-found.
 */
PlayerEntry getEntryFromUserName(std::string uname);

/**
 * @brief Player uname requests the list of open lobbies.
 * If they are marked as open_lobby, get rid of that 'cause they shouldn't be.
 * Don't return themselves.
 */
std::vector<std::string> getLobbyList();


/**
 * SAFETY FEATURES
 */

void handleSigint(int signal_num);

/**
 * @brief Close every socket in a file descriptor set.
 */
void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd);

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
 * @brief Simple class to generate random values within
 * the valid ID range.
 */
class IdGenerator {
 public:
  std::random_device rd;
  std::mt19937_64 gen;
  std::uniform_int_distribution<uint64_t> dist;

  IdGenerator() {
    gen = std::mt19937_64(rd());
    dist = std::uniform_int_distribution<uint64_t>(MIN_ID_VALUE, MAX_ID_VALUE);
  }

  uint64_t getRandomId() {
    return dist(gen);
  }

} id_generator;