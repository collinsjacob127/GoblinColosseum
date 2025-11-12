/**
 * Definitions for local network functions
 * Built off of starter code from:  https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 * and: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
 */

#include "net.hpp"

#ifdef _WIN32
/**
 * NET ENGINE WINDOWS DEFINITIONS
 */

NetEngine::NetEngine() {
  server_sock = -1;
  peer_sock = -1;
}

NetEngine::~NetEngine() {
  serverDisconnect();
  peerDisconnect();
}

int NetEngine::serverConnect() {
  WSADATA wsaData;
  ServerSocket = INVALID_SOCKET;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  int iResult;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    return -1;
  }
  
  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  char port_buf[6];
  snprintf(port_buf, sizeof(port_buf), "%d", SERVER_PORT);
  port_buf[5] = '\0';

  std::cout << "Connecting to server on socket " << port_buf << std::endl;

  // Resolve the server address and port
  iResult = getaddrinfo(SERVER_ADDR, port_buf, &hints, &result);
  if ( iResult != 0 ) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return -1;
  }

  // Attempt to connect to an address until one succeeds
  for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
    // Create a SOCKET for connecting to server
    ServerSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ServerSocket == INVALID_SOCKET) {
      printf("socket failed with error: %ld\n", WSAGetLastError());
      WSACleanup();
      return -1;
    }

    // Connect to server.
    iResult = connect( ServerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(ServerSocket);
      ServerSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  if (ServerSocket == INVALID_SOCKET) {
    printf("connection failed with error: %ld\n", WSAGetLastError());
    WSACleanup();
    return -1;
  }

  server_sock = (int)ServerSocket;
  return (int)ServerSocket;
}

ssize_t NetEngine::sendClientPacket(ClientPacket out_pkt) {
  if (ServerSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return -1;
  }

  unsigned char buf[BUFFER_SIZE];

  // Populate buffer with packet contents (prepped for netsend)
  ssize_t pkt_size = out_pkt.buildPacket(buf);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(ServerSocket, (char*)buf+total_bytes_sent, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::stringstream ss;
        ss << "[Error] Failed to send packet: ";
        ss << out_pkt.getStringFromBuffer(buf, pkt_size);
        perror(ss.str().c_str());
      }
      return bytes_sent;
    }
  }

  // int iResult;
  // shutdown the connection since no more data will be sent
  // iResult = shutdown(ServerSocket, SD_SEND);
  // if (iResult == SOCKET_ERROR) {
  //   printf("shutdown failed with error: %d\n", WSAGetLastError());
  //   closesocket(ServerSocket);
  //   WSACleanup();
  //   return -1;
  // }

  return total_bytes_sent;
}

ServerPacket NetEngine::recvServerPacket(int s) {
  if (s != ServerSocket) {
    std::cout << "[Error] ServerSocket mismatch with socket s\n";
  }
  if (ServerSocket == INVALID_SOCKET) {
    printf("[Error] Unable to connect to server!\n");
    WSACleanup();
    return ServerPacket(0,0,0,"");
  }

  unsigned char buf[SERVER_PACKET_N_BYTES];

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0;
  while (total_bytes_in < SERVER_PACKET_N_BYTES) {
    bytes_in = recv(ServerSocket, (char*)buf+total_bytes_in, SERVER_PACKET_N_BYTES-total_bytes_in, 0);
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::stringstream ss;
        ss << "[Error] Failed to receive packet from %d" << s << std::endl;
        perror(ss.str().c_str());
      }
      return ServerPacket(0, 0, 0, "");
    }
  }

  // Convert to ClientPacket
  ServerPacket out_pkt(buf, SERVER_PACKET_N_BYTES);
  return out_pkt;
}

void NetEngine::serverDisconnect() {
  if (ServerSocket != INVALID_SOCKET) {
    // cleanup socket
    shutdown(ServerSocket, SD_BOTH);
    closesocket(ServerSocket);
  }
  // Cleanup Windows
  if (WSACleanup() != 0) {
    std::cout << "WSA Cleanup in serverDisconnect threw error: "
    << std::system_category().message(WSAGetLastError()) << std::endl;
  }
}

int NetEngine::peerConnect() {
  // Verify good peer_addr struct
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "Attempting connection with peer @" << peer_addr.rep_str
    << ":" << peer_addr.port << std::endl;
  }
  if (peer_addr.addr == 0) {
    std::cout << "Peer connection failed; Peer addr not yet set.\n";
    return -1;
  }

  // Reset the socket etc.
  WSADATA wsaData;
  PeerSocket = INVALID_SOCKET;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  int iResult;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    return -1;
  }
  
  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Must convert port # to const char*
  char port_buf[6];
  snprintf(port_buf, sizeof(port_buf), "%u", peer_addr.port);
  port_buf[5] = '\0';

  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "Connecting to peer on port " << port_buf << std::endl;
  }

  // Resolve the peer addr & port
  iResult = getaddrinfo(peer_addr.rep_str.c_str(), port_buf, &hints, &result);
  if ( iResult != 0 ) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return -1;
  }

  // Attempt to connect to an address until one succeeds
  for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) {
    // Create a SOCKET for connecting to peer
    PeerSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (PeerSocket == INVALID_SOCKET) {
      printf("socket failed with error: %ld\n", WSAGetLastError());
      WSACleanup();
      return -1;
    }

    // Connect to server.
    iResult = connect(PeerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(PeerSocket);
      PeerSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  // Verify valid socket
  if (PeerSocket == INVALID_SOCKET) {
    printf("connection failed with error: %ld\n", WSAGetLastError());
    WSACleanup();
    return -1;
  }

  // Set and return socket
  peer_sock = (int)PeerSocket;
  return (int)PeerSocket;
}

ssize_t NetEngine::sendPeerSetupPacket(PeerSetupPacket out_pkt) {
  if (PeerSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return -1;
  }

  ssize_t pkt_size = PEER_SETUP_PACKET_SIZE;

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(PeerSocket, out_pkt.packet_buf+total_bytes_sent, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::cout << "[Error] Failed to send packet: " << std::endl;
        out_pkt.printContents();
        std::cout << "[Error] Send failure error message: "
        << std::system_category().message(WSAGetLastError()) << std::endl;
      }
      return bytes_sent;
    }
  }

  return total_bytes_sent;
}

PeerSetupPacket NetEngine::recvPeerSetupPacket() {
  if (PeerSocket == INVALID_SOCKET) {
    printf("[Error] Unable to connect to server!\n");
    WSACleanup();
    return PeerSetupPacket();
  }

  char buf[PEER_SETUP_PACKET_SIZE] = "";

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0, pkt_size = PEER_SETUP_PACKET_SIZE;
  while (total_bytes_in < pkt_size) {
    bytes_in = recv(ServerSocket, buf+total_bytes_in, SERVER_PACKET_N_BYTES-total_bytes_in, 0);
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::cout << "[Error] Recv failure error message: "
        << std::system_category().message(WSAGetLastError()) << std::endl;
      }
      return PeerSetupPacket();
    }
  }

  // Convert to ClientPacket
  PeerSetupPacket in_pkt(buf, pkt_size);
  return in_pkt;
}


void NetEngine::peerDisconnect() {
  if (PeerSocket != INVALID_SOCKET) {
    // cleanup socket
    shutdown(PeerSocket, SD_BOTH);
    closesocket(PeerSocket);
  }
  // Cleanup Windows
  if (WSACleanup() != 0) {
    std::cout << "WSA Cleanup in peerDisconnect threw error: "
    << std::system_category().message(WSAGetLastError()) << std::endl;
  }
}

void crossPlatformSleep(uint32_t milliseconds) {
  Sleep(milliseconds);
}

#else

/**
 * NET ENGINE UNIX DEFINITIONS
 */

NetEngine::NetEngine() {
  server_sock = -1;
  peer_sock = -1;
}

NetEngine::~NetEngine() {
  serverDisconnect();
  peerDisconnect();
}

int NetEngine::serverConnect() {
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

ssize_t NetEngine::sendClientPacket(ClientPacket out_pkt) {
  unsigned char buf[BUFFER_SIZE];

  // Populate buffer with packet contents (prepped for netsend)
  ssize_t pkt_size = out_pkt.buildPacket(buf);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(server_sock, buf, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::stringstream ss;
        ss << "[Error] Failed to send packet: ";
        ss << out_pkt.getStringFromBuffer(buf, pkt_size);
        perror(ss.str().c_str());
      }
      return bytes_sent;
    }
  }
  return total_bytes_sent;
}

ServerPacket NetEngine::recvServerPacket(int s) {
  unsigned char buf[SERVER_PACKET_N_BYTES];

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0;
  while (total_bytes_in < SERVER_PACKET_N_BYTES) {
    bytes_in = recv(s, buf, SERVER_PACKET_N_BYTES-total_bytes_in, 0);
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::stringstream ss;
        ss << "[Error] Failed to receive packet from %d" << s << std::endl;
        perror(ss.str().c_str());
      }
      return ServerPacket(0, 0, 0, "");
    }
  }

  // Convert to ClientPacket
  ServerPacket out_pkt(buf, SERVER_PACKET_N_BYTES);
  return out_pkt;
}

void NetEngine::serverDisconnect() {
  if (server_sock > 0) {
    close(server_sock);
    server_sock = -1;
    if(ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Disconnected from server.\n";
    }
  }
}

int NetEngine::peerConnect() {
  // Verify good peer_addr struct
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "Attempting connection with peer @" << peer_addr.rep_str
    << ":" << peer_addr.port << std::endl;
  }
  if (peer_addr.addr == 0) {
    std::cout << "Peer connection failed; Peer addr not yet set.\n";
    return -1;
  }

  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Connecting to peer on port " << peer_addr.port << std::endl;
  }

  struct sockaddr_in unix_peer_addr;
  unix_peer_addr.sin_family = AF_INET;
  unix_peer_addr.sin_port = htons(peer_addr.port);

  // Creating socket file descriptor
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Creating socket\n";
  }
  if ((peer_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "[Error] Socket creation error" << std::endl;
    return -1;
  }

  // Resolve the peer addr & port
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Verifying address...\n";
  }
  if (inet_pton(unix_peer_addr.sin_family, peer_addr.rep_str.c_str(), &unix_peer_addr.sin_addr) <= 0) {
    std::cerr << "[Error] Invalid address/ Address not supported" << std::endl;
    return -1;
  }

  // Connect to the server
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Attempting connect...\n";
  }
  if (connect(peer_sock, (struct sockaddr*)&unix_peer_addr, sizeof(unix_peer_addr)) < 0) {
      std::cerr << "[Error] Connection Failed" << std::endl;
    return -1;
  } else {
    printf("[Log] Connected to peer!\n");
  }

  // Set and return socket
  return peer_sock;
}

ssize_t NetEngine::sendPeerSetupPacket(PeerSetupPacket out_pkt) {
  if (peer_sock <= 0) {
    printf("Unable to connect to peer!\n");
    peerDisconnect();
    return -1;
  }

  ssize_t pkt_size = PEER_SETUP_PACKET_SIZE;

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(peer_sock, out_pkt.packet_buf+total_bytes_sent, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::cout << "[Error] Failed to send packet: " << std::endl;
        out_pkt.printContents();
      }
      return bytes_sent;
    }
  }

  return total_bytes_sent;
}

PeerSetupPacket NetEngine::recvPeerSetupPacket() {
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  char in_buf[PEER_SETUP_PACKET_SIZE] = "";

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0, pkt_size = PEER_SETUP_PACKET_SIZE;
  while (total_bytes_in < pkt_size) {
    bytes_in = recv(peer_sock, in_buf+total_bytes_in, pkt_size-total_bytes_in, 0);
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_NETCODE_ERROR) {
        std::cout << "[Error] Failed to receive peer setup packet\n";
      }
      return PeerSetupPacket();
    }
  }

  // Convert to ClientPacket
  PeerSetupPacket in_pkt(in_buf, pkt_size);
  return in_pkt;
}

void NetEngine::peerDisconnect() {
  if (peer_sock > 0) {
    close(peer_sock);
    peer_sock = -1;
  }
}

void crossPlatformSleep(uint32_t milliseconds) {
  usleep(1000 * milliseconds);
}

#endif

/**
 * NET ENGINE STATIC DEFINITIONS (Not OS-Specific)
 */

void NetEngine::getLocalUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::getline(std::cin, usr_name);
  size_t n_attempts = 0;
  // If bad input, repeat request until good
  while((usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_SIZE-1
  || usr_name == "LIST"
  || usr_name == "CREATE")
  && n_attempts <= 3) {
    std::cout << "Error] Invalid username: " << usr_name << std::endl;
    std::cout << "Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid.)" << std::endl;
    usr_name = "";
    std::getline(std::cin, usr_name);
    n_attempts++;
  }
  if (n_attempts >= 3) { usr_name = "LIVING FAILURE"; }
  // std::cout << "Username \"" << usr_name << "\" Received!\n";
  // Set the username in NetEngine
  setLocalUserName(usr_name);
}

void NetEngine::setLocalUserName(std::string user_name) {
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

int NetEngine::getLocalJoinOrCreate() {
  int selection = -1;
  std::cout << "Select JOIN or CREATE:" << std::endl;
  std::cout << "[0] JOIN" << std::endl;
  std::cout << "[1] CREATE" << std::endl;
  std::cin >> selection;
  size_t n_attempts = 0;
  while (selection != 0 && selection != 1 && n_attempts < 3) {
    std::cout << std::endl << "Invalid option selected. Please enter 0 or 1." << std::endl;
    std::cin >> selection;
    n_attempts++;
  }
  return selection;
}

ssize_t NetEngine::initializeServerCommunication() {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return -1;
  }
  ClientPacket out_pkt(0, 0, 0, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    perror("[Error] Failed to send lobby creation pkt to server");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.session_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION)
    std::cout << "Server Packet Received:\n" << in_pkt.getStringFromSelf();

  session_id = in_pkt.session_id;

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::createLobby() {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return -1;
  }
  ClientPacket out_pkt(1, session_id, 0, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    perror("[Error] Failed to send lobby creation pkt to server");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION)
    std::cout << "Server Packet Received:\n" << in_pkt.getStringFromSelf();

  lobby_id = in_pkt.lobby_id;

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::getLobbies(size_t min_idx, size_t max_idx) {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return -1;
  }

  ClientPacket out_pkt(2, min_idx, max_idx, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    perror("[Error] Failed to send lobby creation pkt to server");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  lobby_list = in_pkt.parseLobbyList();

  std::cout << "[Log] Received " << lobby_list.size() << " lobbies.\n";
  for (size_t i = 0; i < lobby_list.size(); ++i) {
    std::cout << "  [Lobby " << i << "] "
    << lobby_list.at(i).first << " | "
    << lobby_list.at(i).second << std::endl;
  }

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::joinLobby() {
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return -1;
  }

  ClientPacket out_pkt(3, session_id, lobby_id, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    perror("[Error] Failed to send lobby join pkt to server");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION)
    std::cout << "Server Packet Received:\n" << in_pkt.getStringFromSelf();

  lobby_id = in_pkt.lobby_id;

  serverDisconnect();
  return bytes_sent;
}

clientAddrInfo NetEngine::getPeerAddr() {
  // Clear peer addr
  peer_addr.addr = 0; peer_addr.port = 0;

  // Connect to server
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return clientAddrInfo();
  }

  // Request peer addr
  ClientPacket out_pkt(4, session_id, lobby_id, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    perror("[Error] Failed to send lobby join pkt to server");
    serverDisconnect();
    return clientAddrInfo();
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return clientAddrInfo();
  }

  peer_addr = in_pkt.parseAddrInfo();

  serverDisconnect();
  return peer_addr;
}

PeerSetupPacket NetEngine::initializePeerCommunication(uint16_t game_dur_f, uint8_t character_id) {
  Timer connection_attempt_timer;
  connection_attempt_timer.start();
  // Connect and verify
  std::cout << "[Log] Trying to connect to peer..." << std::endl;
  while (peerConnect() < 0 && connection_attempt_timer.duration() < 10.0) {
    std::cout << "[Error] Connection failed, trying again..." << std::endl;
    crossPlatformSleep(100);
  }
  if (connection_attempt_timer.duration() >= 10.0) {
    peerDisconnect();
    std::cout << "[Error] Connection failed too many times. Exiting.\n";
    return PeerSetupPacket();
  }

  // Send info to peer
  PeerSetupPacket out_pkt(game_dur_f, character_id, username);
  if (sendPeerSetupPacket(out_pkt) < 0) {
    std::cout << "[Error] Failed to send peer setup packet.\n";
    peerDisconnect();
    return PeerSetupPacket();
  } else {
    if (ENABLE_PEERPACKET_INSPECTION) {
      std::cout << "[Log] Successfully sent peer setup packet:" << std::endl;
      out_pkt.printContents();
    }
  }

  // Get info from peer
  PeerSetupPacket in_pkt = recvPeerSetupPacket();
  if (in_pkt.character_id == 0) {
    std::cout << "[Error] Failed to receive peer setup packet\n";
    peerDisconnect();
    return PeerSetupPacket();
  }
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Log] Successfully received peer setup packet:" << std::endl;
    in_pkt.printContents();
  }
  return in_pkt; 
}

int NetEngine::testNetClient() {
  std::cout << "[Log] Running linux netcode" << std::endl;
  Timer timer;
  timer.start();
  ssize_t result;

  // Send username and get session ID
  result = initializeServerCommunication();
  if (result < 0) {
    std::cout << "[Error] Failed to initialize server connection\n";
  }

  // Get user selection
  int is_create = getLocalJoinOrCreate();

  if (is_create) {
    // Create a lobby
    result = createLobby();
    if (result < 0) {
      std::cout << "[Error] Failed to create server\n";
    }
  } else {
    // Receive list of first 10 lobbies
    // Request next 10 lobbies
    size_t n_requested_lobbies = 10;
    size_t min_idx = lobby_list.size();
    size_t max_idx = min_idx + (n_requested_lobbies-1);
    result = getLobbies(min_idx, max_idx);
    if (result < 0) {
      std::cout << "[Error] Failed to get lobby list\n";
      return -1;
    }

    // Select lobby from list
    std::cout << "Select one of the above lobbies (" 
    << min_idx << " - " << max_idx << ")\n";

    int selected_idx = -1;
    while (selected_idx < (int)0 || selected_idx > (int)lobby_list.size()) {
      std::cin >> selected_idx;
      std::cout << "\n";
    }

    // Inform Server of Lobby Join
    lobby_id = lobby_list.at(selected_idx).second;
    joinLobby();
  }

  timer.start();
  std::cout << std::fixed << std::setprecision(2);
  while (peer_addr.addr == 0) {
    std::cout << "[Log] Requesting peer addr ("
    << timer.duration() << "s)" << std::endl;

    // Check if someone has connected to the server
    getPeerAddr();
    if (peer_addr.addr == 0) {
      // Wait 5 seconds
      crossPlatformSleep(5000);
    }
  }

  std::cout << "Peer Addr Received!\n";
  // Get peer info

  // Connect to peer and test communication
  initializePeerCommunication((uint16_t)(60*60*5), CHARACTER_ID_HUNKO);

  // Close peer connection
  peerDisconnect();

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}
