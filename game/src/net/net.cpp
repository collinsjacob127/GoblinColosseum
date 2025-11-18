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
  // Creating socket file descriptor
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    COLORS.printError("[Error] Socket creation error\n");
    return -1;
  }

  // Set connection settings
  struct sockaddr_in serv_addr;
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
    printf("[Log] Connected to server via socket %d\n", server_sock);
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
        COLORS.printError("[Error] Failed to send packet:\n");
        perror("");
        std::stringstream ss;
        ss << out_pkt.getStringFromBuffer(buf, pkt_size);
        std::cout << std::flush << COLORS.cyan_fg;
        std::cout << ss.str().c_str();
        std::cout << COLORS.white_fg;
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
        COLORS.printError(ss.str());
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
    if(ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Disconnected from server.\n";
    }
  }
  server_sock = -1;
}

int NetEngine::updateLocalAddress(int s) {
  // Get local ipv4 https://stackoverflow.com/questions/49335001/get-local-ip-address-in-c

  // Use getsockname to read our local ipv4 addr
  struct sockaddr_in loc_addr;
  socklen_t name_len = sizeof(loc_addr);
  int err = getsockname(s, (struct sockaddr*)&loc_addr, &name_len);

  // Convert provided ipv4 addr
  char buf[80] = "";
  const char* p = inet_ntop(AF_INET, &loc_addr.sin_addr, buf, 80);
  buf[79] = '\0';

  // Print out the new addr
  if (p != NULL) {
    // std::cout << "[Log] Local IPv4 is: " << buf << ":" << ntohs(loc_addr.sin_port) << std::endl;
  } else {
    COLORS.printError("[Error] Failed to retrieve local IPv4 addr\n");
    return -1;
  }

  // Update my_local_addr
  my_local_addr.addr = unpacku32((unsigned char*)&loc_addr.sin_addr);
  my_local_addr.port = ntohs(loc_addr.sin_port);
  std::stringstream ss;
  ss << (int)(((unsigned char*)&my_local_addr.addr)[3]);
  ss << ".";
  ss << (int)(((unsigned char*)&my_local_addr.addr)[2]);
  ss << ".";
  ss << (int)(((unsigned char*)&my_local_addr.addr)[1]);
  ss << ".";
  ss << (int)(((unsigned char*)&my_local_addr.addr)[0]);
  ss << ":" << my_local_addr.port;
  my_local_addr.rep_str = ss.str();
  // std::cout << "[Log] Local IPv4 saved as: " << my_local_addr.rep_str << std::endl;
  return 1;
}

sockaddr_in convertClientAddrInfoToSockAddr(clientAddrInfo cur_addr) {
  // Set connection settings
  struct sockaddr_in out_addr;
  out_addr.sin_family = AF_INET;
  out_addr.sin_port = htons(cur_addr.port);

  // std::cout << "[TEMP] Copying \"" << cur_addr.getIPv4().c_str() << "\" into buffer\n";
  // Pull ipv4 addr from cur_addr
  char out_addr_buf[CLIENT_CONTENTS_SIZE] = "";
  strcpy(out_addr_buf, cur_addr.getIPv4().c_str());
  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, out_addr_buf, &out_addr.sin_addr) < 0) {
    std::stringstream ss;
    ss << "[Error] Invalid address / Address not supported: " 
    << cur_addr.getIPv4().c_str() << std::endl;
    COLORS.printError(ss.str());
  }
  return out_addr;
}

clientAddrInfo convertSockAddrToClientAddrInfo(struct sockaddr_in cur_addr) {
  clientAddrInfo out_addr(cur_addr.sin_addr.s_addr, cur_addr.sin_port);
  return out_addr;
}

int NetEngine::initPeerSocket() {
  if (peer_sock > 0) { close(peer_sock); peer_sock = -1; }
  if (server_sock < 0) {
    if (serverConnect() < 0) { return (peer_sock = -1); }
    peer_sock = updateLocalAddress(server_sock);
    serverDisconnect();
    if (peer_sock < 0) { return (peer_sock = -1); }
  } else {
    updateLocalAddress(server_sock);
  }

  // current port used for outbound to server
  uint16_t some_port = unpacku16((unsigned char*)&my_local_addr.port); 
  std::stringstream ss;
  ss << some_port;

  // Get a UDP socket
  if ((peer_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    COLORS.printError("[Error] Failed to get socket fd for peer.\n");
    return (peer_sock = -1);
  }

  // Enable safe reuse of port
  int opt = 1;
  if (setsockopt(peer_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("[Error] setsockopt\n");
    close(peer_sock);
    return (peer_sock = -1);
  }

  // Set non-blocking
  if (fcntl(peer_sock, F_SETFL, O_NONBLOCK) < 0) {
    perror("[Error] fcntl\n");
    close(peer_sock);
    return (peer_sock = -1);
  }

  struct sockaddr_in loc_addr = convertClientAddrInfoToSockAddr(my_local_addr);

  // Bind to the same local address as was used for server comms
  if (bind(peer_sock, (struct sockaddr*)&loc_addr, sizeof(loc_addr)) < 0) {
    COLORS.printError("[Error] Failed to bind\n");
    close(peer_sock);
    return (peer_sock = -1);
  }

  if (updateLocalAddress(peer_sock) < 0) {
    return (peer_sock = -1);
  }
  std::cout << "[Log] Peer Socket after initPeerSocket(): " << peer_sock << std::endl;

  return peer_sock;
}

int NetEngine::attemptSinglePeerConnection(clientAddrInfo address) {
  // unix_peer_addr.sin_family = AF_INET;

  // Set addr info in UNIX net addr struct
  struct sockaddr_in unix_peer_addr;
  unix_peer_addr.sin_family = AF_INET;
  packi32((unsigned char*)&unix_peer_addr.sin_addr.s_addr, address.addr);
  packi16((unsigned char*)&unix_peer_addr.sin_port, address.port);
  // unix_peer_addr.sin_port = htons(address.port);

  // Creating socket file descriptor
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Creating socket\n";
  }
  // Set socket with UDP
  if ((peer_sock = socket(unix_peer_addr.sin_family, SOCK_DGRAM, 0)) < 0) {
    COLORS.printError("[Error] Socket creation error\n");
    perror("socket");
    return -1;
  }

  int opt = 1;
  // Enable safe reuse of port
  if (setsockopt(peer_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    close(peer_sock);
    return -1;
  }

  // Set non-blocking
  if (fcntl(peer_sock, F_SETFL, O_NONBLOCK) < 0) {
    perror("fcntl");
    close(peer_sock);
    return -1;
  }

  // Connect to the server
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[Debug] Requesting connection to " << address.rep_str << "\n";
  }
  if (connect(peer_sock, (struct sockaddr*)&unix_peer_addr, sizeof(unix_peer_addr)) < 0) {
    COLORS.printError("[Error] Connection Failed\n");
    return -1;
  } else {
    printf("[Log] Connected to peer via socket %d!\n", peer_sock);
  }

  return peer_sock;
}

int NetEngine::peerConnect() {
  // Verify peer addrs
  if (ENABLE_PEERPACKET_INSPECTION) {
    std::cout << "[PEER-CONNECT] Attempting connection with peer:" << std::endl;
    std::cout << "  [ADDR] Public: " << peer_addr_public.rep_str << std::endl;
    std::cout << "  [ADDR] Private: " << peer_addr_private.rep_str << std::endl;
  }
  if (peer_addr_public.addr == 0 || peer_addr_private.addr == 0) {
    std::cout << "Peer connection failed; Peer addr not yet set.\n";
    return -1;
  }

  struct sockaddr_in peer_sockaddr_pub = convertClientAddrInfoToSockAddr(peer_addr_public);
  struct sockaddr_in peer_sockaddr_priv = convertClientAddrInfoToSockAddr(peer_addr_public);

  // Attempt connection max_attempts times
  size_t n_attempts = 0, max_attempts = 15;
  while (n_attempts < max_attempts) {
    if (!continue_program) { exit(0); }

    n_attempts++;
    std::cout << "[PEER-CONNECT] Attempt #" << n_attempts << "...\n";

    // Link socket to peer's public address
    if (connect(peer_sock, (struct sockaddr*)&peer_sockaddr_pub, sizeof(peer_sockaddr_pub)) < 0) {
      COLORS.printError("[Error] Connect call Failed\n");
      return peer_sock;
    }
    
    



    // Link socket to peer's private address
    if (connect(peer_sock, (struct sockaddr*)&peer_sockaddr_priv, sizeof(peer_sockaddr_priv)) < 0) {
      return peer_sock;
    }


    // Wait 1s between connection attempts after the first
    if (n_attempts) { crossPlatformSleep(1000); }
  }

  // Return peer socket
  return peer_sock;
}

ssize_t NetEngine::sendPeerSetupPacket(PeerSetupPacket out_pkt, clientAddrInfo some_addr) {
  if (peer_sock <= 0) {
    COLORS.printError("[Error] Invalid peer connection!\n");
    peerDisconnect();
    return -1;
  }

  ssize_t pkt_size = PEER_SETUP_PACKET_SIZE;

  struct sockaddr_in peer_sockaddr = convertClientAddrInfoToSockAddr(some_addr);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = sendto(
      peer_sock, // fd
      out_pkt.packet_buf+total_bytes_sent,  // buffer
      pkt_size-total_bytes_sent, // bytes to send
      0, // flags
      (struct sockaddr*)&peer_sockaddr, // sockaddr
      sizeof(peer_sockaddr)// socklen
    );
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

std::pair<PeerSetupPacket,clientAddrInfo> NetEngine::recvPeerSetupPacket() {
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  char in_buf[PEER_SETUP_PACKET_SIZE] = "";

  // sendto() - For UDP
  // recvfrom() - For UDP & Safer

  // Make UDP port before sending server the addrs
  // Send server the UDP port info

  // - Maybe reimplement ARQ for sending inputs

  // bind() - Guarantee my own port # maybe bad

  struct sockaddr_in peer_sockaddr;
  socklen_t sockaddr_len = sizeof(peer_sockaddr);

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0, pkt_size = PEER_SETUP_PACKET_SIZE;
  while (total_bytes_in < pkt_size) {
    bytes_in = recvfrom(
      peer_sock, // fd
      in_buf+total_bytes_in,  // buffer
      pkt_size-total_bytes_in, // bytes to send
      0, // flags
      (struct sockaddr*)&peer_sockaddr, // sockaddr
      &sockaddr_len// socklen
    );
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_NETCODE_ERROR) {
        COLORS.printError("[Error] Failed to receive peer setup packet\n");
      }
      return std::pair<PeerSetupPacket,clientAddrInfo>(PeerSetupPacket(),convertSockAddrToClientAddrInfo(peer_sockaddr));
    }
  }

  // Convert to ClientPacket
  PeerSetupPacket in_pkt(in_buf, pkt_size);
  return std::pair<PeerSetupPacket,clientAddrInfo>(in_pkt,convertSockAddrToClientAddrInfo(peer_sockaddr));
}

void NetEngine::peerDisconnect() {
  if (peer_sock > 0) {
    close(peer_sock);
    peer_sock = -1;
    if(ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Disconnected from peer.\n";
    }
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
  while(usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_SIZE-1
  || usr_name == "LIST"
  || usr_name == "CREATE") {
    if (!continue_program) { exit(0); }

    std::stringstream ss;
    ss << "[Error] Invalid username: " << usr_name << std::endl;
    ss << "[Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid.)" << std::endl;
    COLORS.printError(ss.str());

    usr_name = "";
    std::getline(std::cin, usr_name);
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
    COLORS.printError(ss.str());
  }
}

int NetEngine::getLocalJoinOrCreate() {
  int selection = -1;
  std::cout << std::flush << COLORS.brt_white_fg;
  std::cout << "Select JOIN or CREATE:" << std::endl;
  std::cout << "[0] JOIN" << std::endl;
  std::cout << "[1] CREATE" << std::endl;
  std::cin >> selection;
  while (selection != 0 && selection != 1) {
    if (!continue_program) { exit(0); }
    std::cout << std::endl << "Invalid option selected. Please enter 0 or 1." << std::endl;
    std::cin >> selection;
  }
  std::cout << COLORS.white_fg;
  return selection;
}

void NetEngine::printLobbyList() {
  std::cout << std::flush << COLORS.brt_white_fg;
  std::cout << "\n[-- AVAILABLE LOBBIES BELOW --]\n";

  for (size_t i = 0; i < lobby_list.size(); ++i) {
    std::cout << "[" << i << "] "
    << lobby_list.at(i).first << " ("
    << lobby_list.at(i).second << ")" << std::endl;
  }

  if (lobby_list.size() <= 0) {
    std::cout << COLORS.yellow_fg;
    std::cout << "[Warning] There are currently no open lobbies.\n";
    std::cout << COLORS.white_fg << std::flush;
  }

  std::cout << COLORS.white_fg;
}

ssize_t NetEngine::initializeServerCommunication() {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    COLORS.printError("[Error] Server connect request failed\n");
    serverDisconnect();
    return -1;
  }
  ClientPacket out_pkt(0, 0, 0, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    COLORS.printError("[Error] Failed to send lobby creation pkt to server\n");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.session_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION) {
    std::cout << "[Packet] Server Packet Received:\n";
    std::cout << std::flush << COLORS.cyan_fg;
    std::cout << in_pkt.getStringFromSelf();
    std::cout << COLORS.white_fg;
  }

  session_id = in_pkt.session_id;

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::createLobby() {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    COLORS.printError("[Error] Server connect request failed\n");
    serverDisconnect();
    return -1;
  }

  // Update local addr info for server send AND initialize socket for p2p
  if (initPeerSocket() < 0) {
    serverDisconnect();
    return -1;
  }

  ClientPacket out_pkt(1, session_id, 0, my_local_addr.rep_str.c_str());
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    COLORS.printError("[Error] Failed to send lobby creation pkt to server\n");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION) {
    std::cout << "[Packet] Server Packet Received:\n";
    std::cout << std::flush << COLORS.cyan_fg;
    std::cout << in_pkt.getStringFromSelf();
    std::cout << COLORS.white_fg;
  }

  lobby_id = in_pkt.lobby_id;

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::getLobbies(size_t min_idx, size_t max_idx) {
  // 1. Bind server connection to the returned socket
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    COLORS.printError("[Error] Server connect request failed\n");
    serverDisconnect();
    return -1;
  }

  ClientPacket out_pkt(2, min_idx, max_idx, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    COLORS.printError("[Error] Failed to send lobby creation pkt to server\n");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  lobby_list = in_pkt.parseLobbyList();

  std::cout << "[Log] Received " << lobby_list.size() << " lobbies.\n";

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::joinLobby() {
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    COLORS.printError("[Error] Server connect request failed\n");
    serverDisconnect();
    return -1;
  } 
  
  // Update local addr info for server send AND initialize socket for p2p
  if (initPeerSocket() < 0) {
    serverDisconnect();
    return -1;
  }

  ClientPacket out_pkt(3, session_id, lobby_id, my_local_addr.rep_str.c_str());
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    COLORS.printError("[Error] Failed to send lobby join pkt to server\n");
    serverDisconnect();
    return -1;
  }
  
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return -1;
  }
  if (ENABLE_PACKET_INSPECTION) {
    std::cout << "[Packet] Server Packet Received:\n";
    std::cout << std::flush << COLORS.cyan_fg;
    std::cout << in_pkt.getStringFromSelf();
    std::cout << COLORS.white_fg;
  }

  lobby_id = in_pkt.lobby_id;

  serverDisconnect();
  return bytes_sent;
}

ssize_t NetEngine::getPeerAddr() {
  // Clear peer addrs
  peer_addr_public.addr = 0;
  peer_addr_public.port = 0;
  peer_addr_private.addr = 0;
  peer_addr_private.port = 0;

  // Connect to server
  int result = serverConnect();
  if (server_sock < 0 || result < 0) {
    COLORS.printError("[Error] Server connect request failed\n");
    serverDisconnect();
    return -1;
  }

  // Request peer addr
  ClientPacket out_pkt(4, session_id, lobby_id, username);
  ssize_t bytes_sent = sendClientPacket(out_pkt);
  if (bytes_sent < 0) {
    COLORS.printError("[Error] Failed to send lobby join pkt to server\n");
    serverDisconnect();
    return -1;
  }
  
  // Receive peer addr
  ServerPacket in_pkt = recvServerPacket(server_sock);
  if (in_pkt.lobby_id == 0) {
    serverDisconnect();
    return -1;
  }
  
  // Parse the addrs
  std::pair<clientAddrInfo, clientAddrInfo> in_addrs;
  in_addrs = in_pkt.parseAddrInfo();
  peer_addr_public = in_addrs.first;
  peer_addr_private = in_addrs.second;

  serverDisconnect();
  return bytes_sent;
}

PeerSetupPacket NetEngine::initializePeerCommunication(uint16_t game_dur_f, uint8_t character_id) {
  // Packet for initializing peer communication
  PeerSetupPacket out_pkt(false, game_dur_f, character_id, username); // Sent BEFORE connected
  PeerSetupPacket good_pkt(true, game_dur_f, character_id, username); // Sent AFTER connected
  PeerSetupPacket *pkt_to_send = &out_pkt;

  PeerSetupPacket in_pkt;
  PeerSetupPacket in_pkt_good;

  bool received_anything = false;

  //TODO: Make sure they send one more after connection established (another send after loop)
  // TODO: Check potential issue with bind interfering with message receipt? Bind to public?
  size_t n_attempts = 0, max_attempts = 3;
  // Switch between attempting connections with public & private addrs
  while (n_attempts < max_attempts && !peer_connection_established) {
    n_attempts++;
    if (received_anything) { 
      // Don't switch addr after received_anything
      pkt_to_send = &good_pkt; 
    }

    // Attempt send to pub
    if (sendPeerSetupPacket(*pkt_to_send, peer_addr_public) < 0) {
      // UDP send should never fail
      COLORS.printError("[Error] Failed to send peer setup packet.\n");
      peerDisconnect();
      return PeerSetupPacket();
    } else {
      if (ENABLE_PEERPACKET_INSPECTION) {
        std::cout << "[Log] Successfully sent peer setup packet with connection established: " 
        << ((pkt_to_send->connection_established == 1) ? "true" : "false") << std::endl;
      }
    }
    // Attempt send to priv
    if (sendPeerSetupPacket(*pkt_to_send, peer_addr_private) < 0) {
      // UDP send should never fail
      COLORS.printError("[Error] Failed to send peer setup packet.\n");
      peerDisconnect();
      return PeerSetupPacket();
    } else {
      if (ENABLE_PEERPACKET_INSPECTION) {
        std::cout << "[Log] Successfully sent peer setup packet with connection established: " 
        << ((pkt_to_send->connection_established == 1) ? "true" : "false") << std::endl;
      }
    }

    std::cout << std::endl;

    if (received_anything) {
      peer_connection_established = true;
      // COLORS.printSuccess("[Log] Finished initializing peer packets. Connection should be fully established.\n");
      break;
    }

    crossPlatformSleep(1000);

    // Check received from peer's public addr
    std::pair<PeerSetupPacket, clientAddrInfo> received_info = recvPeerSetupPacket();
    // Parse received packet & addr
    in_pkt = received_info.first;
    clientAddrInfo tmp_recvd_addr = (received_info.second);

    // Check address of incoming packet
    if (tmp_recvd_addr.addr == peer_addr_public.addr) {
      // Address matches peer public
      received_anything = true;
      peer_addr_final = peer_addr_public;
      COLORS.printSuccess("[Log] Received packet from peer public addr:\n");
      in_pkt.printContents();
    } else if (tmp_recvd_addr.addr == peer_addr_private.addr) {
      // Address matches peer private
      received_anything = true;
      peer_addr_final = peer_addr_private;
      COLORS.printSuccess("[Log] Received packet from peer private addr:\n");
      in_pkt.printContents();
    } else {
      // Address did not match either expected, begone
      COLORS.printWarning("[Warn] Received packet from unknown address, discarding\n");
      continue;
    }

    // Check contents of incoming packet
    if (in_pkt.connection_established) {
      sent_post_established = true;
      received_anything = true;
      in_pkt_good = in_pkt;
    } else if ((std::string)in_pkt.user_name == "") {
      COLORS.printWarning("[Warn] Peer initialization packet received containing invalid username.\n");
    } else { 
      received_anything = true; 
    }
  }
  return in_pkt; 
}

int NetEngine::testNetClient() {
  std::cout << "\n[Log] Running linux netcode" << std::endl;
  Timer timer;
  timer.start();
  ssize_t result;

  // Send username and get session ID
  std::cout << "\n[Log] Initializing server communication (requesting session id)\n";
  result = initializeServerCommunication();
  if (result < 0) {
    COLORS.printError("[Error] Failed to initialize server connection\n");
    serverDisconnect();
    return -1;
  }
  std::cout << std::endl;

  // Get user selection
  int is_create = getLocalJoinOrCreate();
  if (is_create) {
    // Create a lobby
    std::cout << "\n[Log] Requesting the server to create a lobby for us...\n";
    result = createLobby();
    if (result < 0) {
      COLORS.printError("[Error] Failed to create lobby\n");
      serverDisconnect();
      return -1;
    }
  } else {
    // Receive list of first 10 lobbies
    // Request next 10 lobbies
    size_t n_requested_lobbies = 10;
    size_t min_idx = lobby_list.size();
    size_t max_idx = min_idx + (n_requested_lobbies-1);

    // Get lobby list from server
    std::cout << "\n[Log] Requesting list of lobbies...\n";
    result = getLobbies(min_idx, max_idx);
    if (result < 0) {
      COLORS.printError("[Error] Failed to get lobby list\n");
      return -1;
    }

    // Display to user
    printLobbyList();
    if (lobby_list.size() <= 0) {
      return -1;
    }

    // Update n lobbies
    max_idx = lobby_list.size()-1;

    // Ask user for input
    std::cout << "\nSelect one of the above lobbies (" 
    << min_idx << " - " << max_idx << ")\n";

    // Retrieve and verify user input
    int selected_idx = -1;
    while (selected_idx < (int)0 || selected_idx > (int)lobby_list.size()) {
      if (!continue_program) { exit(0); }
      std::cin >> selected_idx;
      std::cout << std::flush;
    }

    // Set ID of selected lobby
    lobby_id = lobby_list.at(selected_idx).second;

    // Inform Server of Lobby Join
    std::cout << "\n[Log] Sending server join request for lobby #"
    << selected_idx << ": \"" << lobby_list.at(selected_idx).first << "\"\n";
    if (joinLobby() < 0) { return -1; }
  }

  // Lobby creation & joining is now finished.
  // Begin requesting addrs of matched peer

  // Time how long it takes to match with the peer
  timer.start();
  // Neat printouts
  std::cout << std::fixed << std::setprecision(2);
  // Stop searching once the addr is set
  bool bad_addrs = (peer_addr_public.addr == 0 && peer_addr_private.addr == 0);

  // Continuously request addr of matched peer
  while (bad_addrs) {
    if (!continue_program) { exit(0); }
    std::cout << "\n[Log] Requesting peer addr (" << timer.duration() << "s)" << std::endl;

    // Check if someone has connected to the server
    getPeerAddr();

    // Update our condition
    bad_addrs = (peer_addr_public.addr == 0 && peer_addr_private.addr == 0);
    if (!bad_addrs) { break; }

    // Repeatedly ask the server, waiting 3s in-between
    crossPlatformSleep(3000);
  }

  // Display success statement
  COLORS.printSuccess("\nSuccessfully got peer addr from server!\n");

  // Initiate p2p (udp holepunch - https://bford.info/pub/net/p2pnat/)

  // Connect to peer and test communication
  std::cout << "\n[Log] Initializing peer communication\n";
  initializePeerCommunication((uint16_t)(60*60*5), CHARACTER_ID_HUNKO);

  std::cout << "[TEMP] Peer socket fd: " << peer_sock << std::endl;
  std:: cout << "[TEMP] Local addr: " << my_local_addr.rep_str << std::endl;

  // Close peer connection
  peerDisconnect();

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}
