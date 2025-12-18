/**
 * Author: Jacob Collins
 * 
 * Description:
 * Header for network functionality. 
 * Some networking is overloaded in net.cpp with platform-specific implementations.
 * Bro is the goat: https://beej.us/guide/bgnet/html/index-wide.html
 * Referenced this for p2p nat: https://bford.info/pub/net/p2pnat/
 * 
 * High-level description:
 * - Each send/recv is done as a pair in a single connection instance.
 * - ClientPacket structs are sent to the server and ServerPacket structs are received.
 * 
 * Client / Server interactions use TCP
 * Peer / Peer interactions use UDP
 */

#pragma once

// This makes while statements safe with the signal handler
#include <atomic>
static volatile std::atomic<bool> continue_program(true);
// volatile std::atomic<bool> continue_program(true);

#ifdef _WIN32


#include <winsock2.h>     // addrinfo
#include <ws2tcpip.h>
#include <stdlib.h>       // misc helper funcs
#include <cstdint>         // uintX_t
#include <basetsd.h>       // SSIZE_T

#include <windows.h>       // Windows types

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

SOCKET ServerSocket = INVALID_SOCKET;
SOCKET PeerSocket = INVALID_SOCKET;

typedef SSIZE_T ssize_t;

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> // addrinfo
#include <fcntl.h> // For nonblocking
#include <memory>

#endif

#include <iostream>
#include <string.h>
#include <sstream>
#include <cstring>
// #include <cstdlib> // atexit

#include "engine.hpp"
#include "util.hpp"
#include "packets.hpp"

#define ENABLE_NETCODE_DEBUG false
#define ENABLE_NETCODE_ERROR false
#define ENABLE_NETCODE_LOG false
#define ENABLE_PACKET_INSPECTION false

#define ENABLE_DENSE_PACKET_INSPECTION false

// IPV4 addr of the server
// constexpr char *SERVER_ADDR = (char*)"192.168.1.100"; // Server PC
constexpr char *SERVER_ADDR = (char*)"35.227.175.118"; // Server PC
// constexpr char *SERVER_ADDR = (char*)"192.168.1.133"; // Desktop - public
// constexpr char *SERVER_ADDR = (char*)"172.25.116.169"; // Desktop - private
// constexpr char *SERVER_ADDR = (char*)"127.0.0.1";
constexpr int SERVER_PORT = 53243;

/**
 * @brief Driver class for all client-side network activity.
 */
class NetEngine {
 public:
  std::string username = "";
  std::string warning_text = "";

  uint16_t p_num = 0;

  uint64_t session_id = 0;
  uint64_t lobby_id = 0;
  // std::vector<std::string> lobby_list = {};
  std::vector<TYPE_LOBBY_INFO> lobby_list = {};

  clientAddrInfo my_local_addr;
  clientAddrInfo peer_addr_private;
  clientAddrInfo peer_addr_public;
  bool peer_connection_established = false;
  bool game_started = false;
  bool game_finished = false;
  clientAddrInfo peer_addr_final;

  // Server connection is 2-way on this socket
  int server_sock = -1;

  // Peers both listen and send to eachother
  int peer_sock = -1;
  // int peer_sock_listen = -1;
  // int peer_sock_send = -1;

  NetEngine();
  ~NetEngine();

  /**
   * @brief Function to get a user's name from cin.
   * @note Verifies that the name is valid and sets the username
   */
  void getLocalUserName();

  /**
   * @brief Get if p1 or p2
   */
  void getPlayerSelection();

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
   * @brief Function to display the list of lobbies to the terminal
   */
  void printLobbyList();

  /**
   * @brief Function to test client-side network functionality
   * @note It is assumed that the username has already been set
   */
  int testNetClient();

  /**
   * @brief Function to test the packing of buttons
   */
  int testButtonPacket(const ButtonStates *buttons);

  /*
  SERVER - CLIENT FUNCTIONS 
  */

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

  /**
   * @brief Function to inform the server of intent to join
   * lobby with corresponding SID
   * @return Bytes sent, or -1 on failure
   */
  ssize_t joinLobby();

  /**
   * @brief Function to get a matchmade peer's addr information
   * from the server.
   * @return Sets peer_addr_public and peer_addr_private based on
   * the server's response
   */
  ssize_t getPeerAddr();

  /*
  PEER - PEER FUNCTIONS
  */

  /**
   * @brief Function to initialize a game between two peers
   * after setup via server.
   * @return The packet received from the peer
   * @note Repeatedly attempts to send & recv the initialization packets
   * to & from the peer's public & private addresses
   */
  PeerSetupPacket initializePeerCommunication(uint16_t game_dur_f, uint8_t character_id);

  /**
   * @brief Build a packet with these inputs and send it
   * @note Packet requests should be handled by `requestPeerInputs()`
   */
  ssize_t sendNetInputs(NetInputs inputs);

  /**
   * @brief Check for incoming peer input packets.
   * @note If receiving an invalid packet, ignore and flush it
   * @return pair: ( T/F - is packet valid ) ( NetInputs packet )
   */
  std::pair<bool, NetInputs> recvPeerInputs();

  /**
   * @brief Alert a peer that we need inputs of frame `f_num` resent.
   */
  ssize_t requestPeerResendInputs(uint16_t f_num);

 private:

 /**
  * @brief Function to clear the queue of the UDP buffer used for p2p
  * @note Only use when you know you have time and won't destroy anything
  * @return -1 on failure, else # bytes cleared
  */
 ssize_t clearSocketQueue();

  /**
   * @brief Function to update my_local_addr based on the current connection
   * @return 1 on success, -1 on failure
   * @note References this forum post https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c
   */
  int updateLocalAddress(int s);

  /**
   * @brief Function to create a socket for the peer.
   * @note Sets peer_sock to the returned value.
   * @note Updates my_local_addr with the address bound
   * to this socket.
   * @note CANNOT BE DONE WHILE CONNECTED TO SERVER
   */
  int initPeerSocket();

  /**
   * @brief Function to send a setup packet to the peer
   * @return Returns bytes sent or -1 on error 
   * @note For TCP Holepunch, this will very likely error
   * the first time, and must be sent twice.
   */
  ssize_t sendPeerSetupPacket(PeerSetupPacket, clientAddrInfo);

  /**
   * @brief Function to send peer setup packet to both the public and
   * private addrs of peer.
   * @return -1 on failure or 1 on success
   */
  ssize_t sendPeerSetupBoth(PeerSetupPacket out_pkt);

  /**
   * @brief Function to receive a peer setup packet
   * @return The packet received. All default values on failure.
   */
  std::pair<PeerSetupPacket,clientAddrInfo> recvPeerSetupPacket();

  /**
   * @brief Function to disconnect from a peer
   */
  void peerDisconnect();

  /**
   * @brief Function to connect to the game's server
   */
  int serverConnect();

  /**
   * @brief Function to parse and send a ClientPacket to the server
   * @return Returns bytes sent or -1 on error
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
};

void crossPlatformSleep(uint32_t milliseconds);
