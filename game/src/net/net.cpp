/**
 * Definitions for local network functions
 * Built off of starter code from:  https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 * and: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
 */

#include "net.hpp"

//DELETE THIS
#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


int testNetClient() {
  std::cout << "Running windows netcode" << std::endl;
  Timer timer;
  timer.start();

  // Do boilerplate winsock stuff 
  WSADATA wsaData;
  int iResult;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  char buffer[BUFFER_SIZE] = {0};

  SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr); // Connect to localhost
  serverAddr.sin_port = htons(SERVER_PORT); // Example port

  connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
  // ... send and receive data with connectSocket ...
  if (connectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
  }

  // Send test packet thrice
  for (int i = 0; i < 3; ++i) {
    // Send one per 2 second
    if (timer.duration() < (double)i * 2) {
      --i;
      continue;
    }

    std::stringstream ss;
    ss << "Hello #" << i << " from Windows client!" << std::endl;
    std::string hello = ss.str();

    iResult = send( connectSocket, hello.c_str(), hello.size(), 0 );
    if (iResult == SOCKET_ERROR) {
      printf("send failed with error: %d\n", WSAGetLastError());
      closesocket(connectSocket);
      WSACleanup();
      return 1;
    }
    printf("Bytes Sent: %ld\n", iResult);
  }


  // shutdown the connection since no more data will be sent
  iResult = shutdown(connectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(connectSocket);
    WSACleanup();
    return 1;
  }

// Receive until the peer closes the connection
  do {

    iResult = recv(connectSocket, buffer, BUFFER_SIZE, 0);
    if ( iResult > 0 ) {
      printf("Bytes received: %d\n", iResult);
      std::cout << "Received: " << buffer << std::endl;
    } else if ( iResult == 0 )
      printf("Connection closed\n");
    else
      printf("recv failed with error: %d\n", WSAGetLastError());

  } while( iResult > 0 );

  closesocket(connectSocket);
  WSACleanup();
  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";
  return 0;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

NetEngine::NetEngine() {
  server_sock = -1;
  peer_sock = -1;
}

NetEngine::~NetEngine() {
  disconnectServer();
  disconnectPeer();
}

int NetEngine::connectToServer() {
  struct sockaddr_in serv_addr;
  // Creating socket file descriptor
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "[Error] Socket creation error" << std::endl;
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
    std::cerr << "[Error] Invalid address/ Address not supported" << std::endl;
    return -1;
  }

  // Connect to the server
  if (connect(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cerr << "[Error] Connection Failed" << std::endl;
    return -1;
  } else {
    printf("[Log] Connected to server\n");
  }
  return 0;
}

std::vector<std::string> NetEngine::receiveServerList() {
  // Initialize list and buffer
  std::vector<std::string> player_list;
  char buffer[BUFFER_SIZE];
  std::string n_lobby_str;
  int n_lobbies;

  // First, server sends a single character with how many lobbies there are
  int n_bytes = 0;
  n_bytes = recv(server_sock, &buffer, sizeof(buffer), 0);
  if (n_bytes < 0) {
    if (ENABLE_NETCODE_ERROR) {
      std::stringstream ss;
      ss << "[Error] Failed to receive number of lobbies being sent\n";
      perror(ss.str().c_str());
      return {};
    }
  } else {
    n_lobby_str = buffer;
    n_lobbies = atoi(n_lobby_str.c_str());
    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Original # lobbies recvd: " << n_lobbies << std::endl;
      std::cout << "[Debug] NTOHS # lobbies recvd: " << ntohs(n_lobbies) << std::endl;
      std::cout << "[Debug] NTOHS # lobbies recvd: " << ntohs(n_lobbies) << std::endl;
    }
    // n_lobbies = ntohs(n_lobbies);
    if(ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Server says to expect " << n_lobbies << " lobby names\n";
    }
  }

  if (n_lobbies <= 0) {
    return {};
  }

  // Receive each peer's username from the server.
  for (int i = 0; i < n_lobbies; ++i) {
    n_bytes = 0;
    n_bytes = recv(server_sock, buffer, MAX_USERNAME_SIZE, 0);
    if (n_bytes < 0) { 
      perror("[Error] Error in receiving server's list"); 
      return {};
    }
    std::string peer_username = buffer;

    std::cout << "[Log] Received username from server list: " 
    << peer_username << " (" << n_bytes << "b)" << std::endl;

    player_list.push_back(peer_username);
  }

  return player_list;
}

int NetEngine::getJoinOrCreate() {
  int selection = -1;
  std::cout << "Select JOIN or CREATE:" << std::endl;
  std::cout << "[0] JOIN" << std::endl;
  std::cout << "[1] CREATE" << std::endl;
  std::cin >> selection;
  while (selection != 0 && selection != 1) {
    std::cout << std::endl << "Invalid option selected. Please enter 0 or 1." << std::endl;
    std::cin >> selection;
  }
  return selection;
}

size_t NetEngine::selectLobby(size_t max_idx) {
  int selection = -1;
  std::cout << "Select peer to join (" << 0 << " - " << max_idx << ")" << std::endl;
  std::cin >> selection;
  while (selection < 0 || selection > max_idx) {
    std::cout << "Invalid selection, please try again.\n";
    std::cin >> selection;
  }
  return 0;
}

int NetEngine::sendJoinRequest(std::string peer_name) {
  // Verify peer name length
  if (!(peer_name.size() < MAX_USERNAME_SIZE)) {
    std::cerr << "[Error] Requesting to join peer with name too large: " 
    << peer_name << std::endl;
  }

  // Initialize lobby request packet
  ClientPacket pkt(0, peer_name.c_str());
  // Send it over
  int n_bytes = send(server_sock, peer_name.c_str(), peer_name.size(), 0);
  // Verify good send
  if (n_bytes < 0) {
    perror("[Error] Lobby join request failed\n");
    return -1;
  }
  // Log
  std::cout << "[Log] Lobby join request sent: " << username << std::endl;
  return 0;
}

std::pair<std::string, std::string> NetEngine::getPeerAddrInfo() {
  char buffer[BUFFER_SIZE];
  int n_bytes = 0;
  std::string raw_addr = "";
  size_t colon_pos = std::string::npos;
  std::pair<std::string, std::string> addr_pair;

  // Receive the address of connecting peer
  if ((n_bytes = recv(server_sock, buffer, BUFFER_SIZE-1, 0)) < 0) {
    perror("[Error] Error in get peer addr info: ");
    return {};
  }
  buffer[n_bytes] = '\0';

  raw_addr = buffer;
  colon_pos = raw_addr.find(':');
  if (colon_pos == std::string::npos) {
    perror("Received bad addr");
    return {};
  }

  if (ENABLE_NETCODE_DEBUG)
    std::cout << "[Debug] Peer addr info ("
    << n_bytes << "b) received: " << raw_addr << std::endl;
  
  // Parse ipv4 addr
  addr_pair.first = raw_addr.substr(0, colon_pos);
  addr_pair.second = raw_addr.substr(colon_pos, raw_addr.size());

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Addr parsed as: " 
    << addr_pair.first << ":" 
    << addr_pair.second << std::endl;
  }

  if (n_bytes < 0) { perror("[Error] Error in receiving server's list"); }
  return addr_pair;
}

int NetEngine::sendUserName() {
  ClientPacket pkt(0, username);
  char buf[sizeof(ClientPacket)];
  pkt.buildSendPacket(buf);
  int n_bytes = send(server_sock, buf, sizeof(buf), 0);
  if (n_bytes < 0) {
    perror("[Error] Username send failed\n");
    return -1;
  }
  if(ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Username packet sent: " << std::string(buf+1) << std::endl;
  }
  memset(buf, 0, sizeof(buf));
  n_bytes = recv(server_sock, buf, MAX_USERNAME_SIZE, 0);
  if (n_bytes < 0) {
    perror("[Error] Server username response not received");
    return 2;
  } else {
    std::string server_feedback = buf;
    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Server responded to username request with " 
      << server_feedback << std::endl;
    }
    if (server_feedback != "GOOD") {
      std::cout << "[Log] Server notified username taken, please try again\n";
      return 2;
    }
  }
  return 0;
}

int NetEngine::sendJoin() {
  // Build packet with necessary info
  ClientPacket pkt(1, "JOIN");
  // Buffer for network packet
  char buf[sizeof(ClientPacket)];
  // Build network packet
  pkt.buildSendPacket(buf);
  // Send and verify bytes
  size_t n_bytes;
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Preparing to send JOIN intent notification" << std::endl;
  }

  n_bytes = send(server_sock, buf, sizeof(buf), 0);
  if (n_bytes < 0) {
    perror("[Error] Failed to send join packet:");
    return -1;
  }
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] JOIN intent notification sent" << std::endl;
  }
  // If max attempts hit, return -1
  return 0;
}

int NetEngine::sendCreate() {
  // Build packet with necessary info
  ClientPacket pkt(1, "CREATE");
  // Buffer for network packet
  char buf[sizeof(ClientPacket)];
  // Build network packet
  pkt.buildSendPacket(buf);
  // Send and verify bytes
  size_t n_bytes, n_attempts = 0, max_attempts = 3;
  n_bytes = send(server_sock, buf, sizeof(buf), 0);

  if (n_bytes < 0) {
    std::cerr << "[Error] Failed to send create packet" << std::endl;
    return -1;
  }
  if(ENABLE_NETCODE_DEBUG) { std::cout << "[Debug] CREATE packet sent\n"; }
  // If max attempts hit, return -1
  return 0; 
}

int NetEngine::testNetClient() {
  std::cout << "[Log] Running linux netcode" << std::endl;
  Timer timer;
  timer.start();
  int result, attempt_cnt, max_attempts = 3;
  char buffer[BUFFER_SIZE] = {0};

  // 1. Bind server connection to the returned socket
  result = connectToServer();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    disconnectServer();
    return -1;
  }


  // 2. Send your username to initialize server connection
  result = -1;
  result = sendUserName();
  if (result == 2) {
    disconnectServer();
    return 2; // Retry username
  }


  // 3. Declare joining or creating a game (JOIN vs CREATE)
  int join_or_create = getJoinOrCreate();

  // 4 (CREATE). Send server notification that you wish to create a lobby
  result = 0;
  if (join_or_create == 1) {
    // Tell server you wish to CREATE a game
    result = sendCreate();
  }
  if (result < 0) {
    std::cerr << "[Error] Failed to send lobby creation message." << std::endl;
    disconnectServer();
    return -1;
  }


  // 4 (JOIN). Send join request and receive list of users in server's registry
  if (join_or_create == 0) {
    std::vector<std::string> player_list = {};

    // Link this to button instead of looping until one is found
    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Preparing to request lobby list from server" << std::endl;
    }

    // Notify of intent to join
    result = sendJoin();
    if (result < 0) {
      std::cerr << "[Error] Failed to send join intent." << std::endl;
      disconnectServer();
      return -1;
    } 

    // Receive list of open lobbies
    player_list = receiveServerList();

    // Select a lobby if there are any
    if (player_list.size() > 0) {
      std::cout << "[Log] Received lobby list: " << std::endl;
      printStringVec(player_list);
      // Send name of user requested OR refresh
      size_t selection = selectLobby(player_list.size()-1);
    } else {
      std::cout << "[Log] No lobbies found." << std::endl;
      disconnectServer();
      return -1;
    }
  }

  // 5. Wait to receive address of connecting peer
  std::pair<std::string, std::string> addr_strs = getPeerAddrInfo(); // blocking
  std::string peer_ipv4 = addr_strs.first;
  std::string peer_port = addr_strs.second;

  std::cout << "Received ipv4: " << peer_ipv4 << ":" << peer_port << std::endl;

  // Close the server connection
  disconnectServer();

  // Connect to peer

  // Test peer connection

  // Close peer connection

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}

void NetEngine::disconnectServer() {
  if (server_sock > 0) {
    close(server_sock);
    server_sock = -1;
  }
}
void NetEngine::disconnectPeer() {
  if (peer_sock > 0) {
    close(peer_sock);
    peer_sock = -1;
  }
}

#endif

ClientPacket::ClientPacket(char type, std::string val) {
  // Set first header value (no need to modify)
  packet_type = type;

  // Set contents (string copy and guarantee null terminator)
  strncpy(contents, val.c_str(), MAX_USERNAME_SIZE-1);
  contents[MAX_USERNAME_SIZE-1] = '\0';

  // Log it
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Log] ClientPacket initialized with " 
    << (int)type << " as type and "
    << contents << " as contents" << std::endl;
  }
}

void ClientPacket::buildSendPacket(char* buf) {
  // Copy packet_type into first byte of network packet
  memcpy(buf, &packet_type, sizeof(packet_type));
  // Copy contents into the rest
  memcpy(buf + sizeof(packet_type), contents, sizeof(contents));
}

void NetEngine::getUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::cin >> usr_name;
  // If bad input, repeat request until good
  while(usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_SIZE-1
  || usr_name == "REFRESH" 
  || usr_name == "JOIN"
  || usr_name == "CREATE") {
    std::cout << "Error] Invalid username: " << usr_name << std::endl;
    std::cout << "Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid." << std::endl;
    usr_name = "";
    std::cin >> usr_name;
  }
  // Set the username in NetEngine
  setUserName(usr_name);
}

void NetEngine::setUserName(std::string user_name) {
  if (user_name.size() < MAX_USERNAME_SIZE) {
    username = user_name;
  } else {
    std::stringstream ss;
    ss << "[Error] ";
    ss << "Net engine failed to set username (" << user_name << ") ";
    ss << "too long (" << user_name.size() << " > " << MAX_USERNAME_SIZE-1 << ")";
    ss << std::endl;
    perror(ss.str().c_str());
  }
}
