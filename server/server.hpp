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
#include <csignal> // Handle SIGINT
#include <memory>
#include <cstring>
#include <netdb.h>
// #include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.hpp"
#include "database.hpp"
#include "packets.hpp"


#define ENABLE_SERVER_LOG true
#define ENABLE_SERVER_DEBUG true
#define ENABLE_SERVER_ERROR true
#define ENABLE_AWAITING_NEW_PACKETS_NOTIF true

constexpr char* SERVER_PORT = (char*)"53243";
constexpr int MAX_PENDING = 64;


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
 * SAFETY FEATURES
 */

void handleSigint(int signal_num);

/**
 * @brief Close every socket in a file descriptor set.
 */
void closeAllConnections();

/**
 * @brief Function to disconnect a client from the server.
 * @note This does NOT remove a player from the registry.
 */
void disconnectClient(int fd);
