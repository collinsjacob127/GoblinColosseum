/**
 * Definitions for local network functions
 * Built off of starter code from:  https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 * and: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
 */

#include "net.hpp"

constexpr int SERVER_PORT = 53243;
constexpr int BUFFER_SIZE = 1024;

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
      std::cout << "Recieved: " << buffer << std::endl;
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

void NetEngine::connectToServer() {
  struct sockaddr_in serv_addr;
  // Creating socket file descriptor
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    return;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return;
  }

  // Connect to the server
  if (connect(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Connection Failed" << std::endl;
    return;
  } else {
    printf("Connected to server\n");
  }
}

std::vector<std::string> NetEngine::recieveServerList() {
  // Initialize list and buffer
  std::vector<std::string> player_list;
  char buffer[BUFFER_SIZE] = {0};

  int n_bytes = -1;
  // Recieve each peer's username from the server.
  while((n_bytes = read(server_sock, buffer, BUFFER_SIZE)) > 0) {
    std::string peer_username = buffer;

    if (ENABLE_NETCODE_DEBUG)
      std::cout << "Recieved username from server list: " << peer_username << std::endl;

    player_list.push_back(peer_username);
  }

  if (n_bytes < 0) { perror("Error in recieving server's list"); }
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

int NetEngine::getJoinInfo(size_t n_lobbies) {
  int selection = -1;
  std::cout << "Select peer to join:";

}

std::pair<std::string, std::string> NetEngine::getPeerAddrInfo() {
  char buffer[BUFFER_SIZE] = {0};
  int n_bytes = -1;
  std::string raw_addr = "";
  std::pair<std::string, std::string> addr_pair;

  // Recieve the address of connecting peer
  while((n_bytes = read(server_sock, buffer, BUFFER_SIZE)) > 0) {
    std::string raw_addr = buffer;

    if (ENABLE_NETCODE_DEBUG)
      std::cout << "[Debug] Peer addr info received: " << raw_addr << std::endl;
    
    size_t colon_pos = raw_addr.find(':');
    addr_pair.first = raw_addr.substr(0, colon_pos);
    addr_pair.second = raw_addr.substr(colon_pos, raw_addr.size());

    if (ENABLE_NETCODE_DEBUG) {
      std::cout << "[Debug] Addr parsed as: " 
      << addr_pair.first << ":" 
      << addr_pair.second << std::endl;
    }
  }

  if (n_bytes < 0) { perror("Error in recieving server's list"); }
}

int NetEngine::testNetClient() {
  std::cout << "Running linux netcode" << std::endl;
  Timer timer;
  timer.start();

  /**
   * Connecting to server
   */
  char buffer[BUFFER_SIZE] = {0};
  // 1. Bind server connection to the returned socket
  connectToServer();
  if (server_sock < 0) {
    perror("Server connect request failed");
    return -1;
  }


  // 2. Send your username to initialize server connection
  int n_bytes = send(server_sock, username.c_str(), username.size(), 0);
  if (n_bytes < 0) {
    perror("Username send failed\n");
    return -1;
  }
  std::cout << "Username sent: " << username << std::endl;


  // 3. Declare joining or creating a game (JOIN vs CREATE)
  int join_or_create = getJoinOrCreate();
  std::string j_c_pkt = "";
  if (join_or_create == 0) {
    // Tell server you wish to JOIN a peer
    j_c_pkt = "JOIN";
    n_bytes = send(server_sock, j_c_pkt.c_str(), j_c_pkt.size(), 0);
  } else if (join_or_create == 1) {
    // Tell server you wish to CREATE a game
    j_c_pkt = "CREATE";
    n_bytes = send(server_sock, j_c_pkt.c_str(), j_c_pkt.size(), 0);
  }
  if (n_bytes < 0) {
    std::cerr << j_c_pkt << " server request failed\n";
    return -1;
  }

  // JOIN ing
  if (join_or_create == 0) {
    // 4 (JOIN ONLY). Display list of users in server's registry
    // (Recieve)
    std::vector<std::string> player_list = recieveServerList();
    std::cout << "Recieved current registry: " << std::endl;
    printStringVec(player_list);

    // Send name of user requested OR refresh
    int selection = getJoinInfo(player_list.size());
  } 
  
  // 5. Wait to recieve address of connecting peer
  std::pair<std::string, std::string> addr_strs = getPeerAddrInfo();
  std::string peer_ipv4 = addr_strs.first;
  std::string peer_port = addr_strs.second;


  // Close the server connection
  close(server_sock);

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

void NetEngine::getUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::cin >> usr_name;
  while(usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_LEN-1 
  || usr_name == "REFRESH" 
  || usr_name == "JOIN"
  || usr_name == "CREATE") {
    std::cout << "Error] Invalid username: " << usr_name << std::endl;
    std::cout << "Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid." << std::endl;
    usr_name = "";
    std::cin >> usr_name;
  }
  username = usr_name;
}
