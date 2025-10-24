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

int NetEngine::sendUserName() {
  ssize_t n_bytes;
  ClientPacket pkt(0, username);
  if ((n_bytes = pkt.sendPacket(server_sock)) < 0) {
    perror("[Error] Username send failed\n");
    return -1;
  } else if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Username sent ("
    << username << " - " << n_bytes << "b)\n";
  }
  
  if ((n_bytes = verifyGoodResponse()) < 0) {
    if (ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Username send failed." << std::endl;
    }
  }

  return n_bytes;
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

int NetEngine::sendCreate() {
  // Build packet with necessary info
  ClientPacket pkt(1, "CREATE");
  ssize_t n_bytes;
  if ((n_bytes = pkt.sendPacket(server_sock))< 0) {
    std::cerr << "[Error] Failed to send create packet" << std::endl;
    return -1;
  }

  if(ENABLE_NETCODE_DEBUG) { std::cout << "[Debug] CREATE packet sent\n"; }

  if ((n_bytes = verifyGoodResponse()) < 0) {
    if (ENABLE_NETCODE_LOG) {
      std::cout << "[Log] CREATE request failed." << std::endl;
    }
    return n_bytes;
  } else {
    if (ENABLE_NETCODE_LOG) {
      std::cout << "[Log] CREATE request succeeded." << std::endl;
    }
  }

  return 0; 
}

std::vector<std::string> NetEngine::receiveServerList() {
  // Build packet with necessary info
  ClientPacket pkt(2, "LIST");
  // Send and verify bytes
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Preparing to request LIST" << std::endl;
  }
  // Send network packet
  ssize_t n_bytes;
  if ((n_bytes  = pkt.sendPacket(server_sock)) < 0) {
    perror("[Error] Failed to send LIST request:");
    return {};
  } else if (ENABLE_NETCODE_LOG) {
    std::cout << "[Log] LIST request sent" << std::endl;
  }

  // Initialize list and buffer
  std::vector<std::string> player_list;
  char buffer[BUFFER_SIZE];
  std::string n_lobby_str;
  int n_lobbies;

  // First, server sends a single character with how many lobbies there are
  if ((n_bytes = recv(server_sock, &buffer, sizeof(buffer), 0)) < 0) {
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
  ssize_t n_bytes = pkt.sendPacket(server_sock);
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
  std::pair<std::string, std::string> addr_pair;
  unsigned char buffer[6]; // ipv4=4 + port=2
  int n_bytes = 0;

  // Sending packet with header 3 - request for peer addr
  ClientPacket out_pkt(3, "");
  if ((n_bytes = out_pkt.sendPacket(server_sock)) < 0) {
    perror("\n[Error] Failed to send request for peer addr\n");
    return {};
  }

  // Receive the address of connecting peer
  if ((n_bytes = recv(server_sock, buffer, 6, 0)) < 0) {
    perror("[Error] Error in get peer addr info: ");
    return {};
  } else {
    if(ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Bytes recieved for peer addr: "
      << n_bytes << std::endl;
    }
  }

  if (n_bytes != 6) {
    if (ENABLE_NETCODE_ERROR) {
      std::cerr << "[Error] Invalid packet size returned on peer addr request\n";
    }
    return {};
  }

  uint32_t peer_addr = ntohl(*buffer);
  uint16_t peer_port = ntohs(*(buffer + 4));

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Addr parsed as: " 
    << peer_addr << ":" 
    << peer_port << std::endl;
  }

  std::stringstream ss_addr;
  std::cout << "[         ] IPV4: " << (uint8_t) *(&peer_addr) << std::endl;
  ss_addr << (uint8_t) *(&peer_addr);
  for (size_t i = 1; i < 4; ++i) {
    std::cout << "[         ] IPV4: " << (uint8_t) *(&peer_addr + i) << std::endl;
    ss_addr << "." << (uint8_t) *(&peer_addr + i);
  }
  addr_pair.first = ss_addr.str();
  std::stringstream ss_port;
  ss_port << peer_port;
  addr_pair.second = ss_port.str();

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Addr parsed as: " 
    << addr_pair.first << ":" 
    << addr_pair.second << std::endl;
  }

  return addr_pair;
}

ssize_t NetEngine::verifyGoodResponse() {
  char serv_cstr[MAX_USERNAME_SIZE];
  ssize_t n_bytes = recv(server_sock, serv_cstr, MAX_USERNAME_SIZE-1, 0);
  if (n_bytes < 0) {
    perror("[Error] Server response not received");
    return -1;
  } else {
  serv_cstr[n_bytes] = '\0';
    std::string server_feedback = serv_cstr;
    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Server responded with " 
      << server_feedback << std::endl;
    }
    if (server_feedback != "GOOD") {
      std::cout << "[Log] Server notified BAD, please try again\n";
      return 1;
    }
  }
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
  if (result == 1) {
    disconnectServer();
    return 1; // Retry username
  }


  // 3. Would you like to view lobbies or create a lobby
  int join_or_create = getJoinOrCreate();


  // 4 (CREATE). Send server notification that you wish to create a lobby
  result = 0;
  if (join_or_create == 1) {
    // Tell server you wish to CREATE a game
    if ((result = sendCreate()) < 0) {
      std::cerr << "[Error] Failed to send lobby creation message." << std::endl;
      disconnectServer();
      return -1;
    };
  }

  // 4 (LIST) Request list of current lobbies from server
  if (join_or_create == 0) {
    std::vector<std::string> player_list = {};

    // Link this to button instead of looping until one is found
    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Preparing to request lobby list from server" << std::endl;
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
  std::pair<std::string, std::string> addr_strs;
  while ((addr_strs = getPeerAddrInfo()).second == "0") {
    std::cout << "[Log] Peer not yet assigned for connection, waiting...\n";
    sleep(2);
  }
  std::string peer_ipv4 = addr_strs.first;
  std::string peer_port = addr_strs.second;

  if (ENABLE_NETCODE_LOG) {
    std::cout << "[Log] Received ipv4: " << peer_ipv4 << ":" << peer_port << std::endl;
  }

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

ssize_t ClientPacket::sendPacket(int fd) {
  contents[MAX_USERNAME_SIZE-1] = '\0';
  // Initialize buffer for packet to send
  char buf[sizeof(ClientPacket)];
  if (ENABLE_CLIENTPACKET_PRESEND_INSPECTION) {
    std::cout << "[PACKET PRE-SEND] type = " << packet_type << std::endl;
    std::cout << "[PACKET PRE-SEND] contents = " << packet_type << std::endl;
    std::cout << "[PACKET PRE-SEND] # bytes = " << sizeof(buf) << std::endl;
  }
  // Copy packet_type into first byte of network packet
  memcpy(buf, &packet_type, sizeof(packet_type));
  // Copy contents into the rest
  memcpy(buf + sizeof(packet_type), contents, sizeof(contents));
  // Send to server
  ssize_t n_bytes;
  return send(fd, buf, sizeof(buf), 0);
}

void NetEngine::getUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::cin >> usr_name;
  // If bad input, repeat request until good
  while(usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_SIZE-1
  || usr_name == "LIST"
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

// void AddrInfoPkt::setFromRaw(unsigned char *buf) {
//   size_t i;
//   // Set ipv4
//   for (i = 0; i < 4; ++i) {
//     (&ipv4_addr)[i] = buf[i];
//   }
//   // Set port
//   (&port_num)[0] = buf[++i];
//   (&port_num)[1] = buf[++i];
// }
