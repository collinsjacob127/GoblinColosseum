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
void printWindowsError(std::string context_name) {
  std::cout << std::flush << COLORS.red_fg;
  std::cout << "[Error] \"" << context_name << "\" threw error: ";
  std::cout << std::system_category().message(WSAGetLastError());
  std::cout << COLORS.white_fg << std::endl;
}

NetEngine::NetEngine() {
  server_sock = -1;
  peer_sock = -1;
  ServerSocket = INVALID_SOCKET;
  PeerSocket = INVALID_SOCKET;

  // Initialize Winsock
  WSADATA wsaData;
  int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
  }
}

NetEngine::~NetEngine() {
  serverDisconnect();
  peerDisconnect();
  // Cleanup Windows
  std::cout << "[Log] Performing WSA Cleanup...\n";
  if (WSACleanup() != 0) {
    std::cerr << "[Error] WSA Cleanup threw error: "
    << std::system_category().message(WSAGetLastError()) << std::endl;
  }
}

int NetEngine::serverConnect() {
  ServerSocket = INVALID_SOCKET;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  int iResult;
  
  ZeroMemory( &hints, sizeof(hints) );
  hints.ai_family = AF_INET;
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
      printWindowsError("create socket");
      return -1;
    }

    // // Values to set options with
    // int iOptVal = 0;
    // int iOptLen = sizeof(int);
    // BOOL bOptVal = TRUE;
    // int bOptLen = sizeof(BOOL);

    // // Enable safe reuse of port
    // // Missing SO_REUSEPORT - winsock doesn't have it, hopefully that's alright
    // // SO_CONDITIONAL_ACCEPT is worth looking into
    // // so is SO_RCVTIMEO & SO_SNDTIMEO
    // if (setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen) == SOCKET_ERROR) {
    //   printWindowsError("setsockopt - SO_REUSEADDR");
    //   serverDisconnect();
    //   return -1;
    // }

    // if (getsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&iOptVal, &iOptLen) == SOCKET_ERROR) {
    //   printWindowsError("getsockopt for SO_REUSEADDR");
    //   serverDisconnect();
    //   return -1;
    // } else {
    //   std::cout << "[TEMP] SO_REUSEADDR Value: " << iOptVal << std::endl;
    // }

    // Connect to server.
    iResult = connect(ServerSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(ServerSocket);
      ServerSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  if (ServerSocket == INVALID_SOCKET) {
    printWindowsError("invalid server socket");
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
        perror("[Error] Failed to send packet: ");
        ss << out_pkt.getStringFromBuffer(buf, pkt_size) << std::endl;
        COLORS.printInColor(ss.str(), COLORS.cyan_fg);
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
    ServerSocket = INVALID_SOCKET;
    server_sock = -1;
  }
}

clientAddrInfo convertSockAddrToClientAddrInfo(sockaddr_in cur_addr) {
  return clientAddrInfo(cur_addr.sin_addr.S_un.S_addr, cur_addr.sin_port);
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

  // Print the converted address 
  // std::stringstream ss;
  // ss << "[TEMP] Address converted from ClientAddrInfo to SockAddr: " 
  // << convertSockAddrToClientAddrInfo(out_addr).rep_str << std::endl;
  // COLORS.printSuccess(ss.str());

  return out_addr;
}

int NetEngine::updateLocalAddress(int s) {
  // safe to cast SOCKET to int: https://www.openssl.org/docs/man3.0/man3/SSL_set_fd.html
  SOCKET this_sock;
  if ((int)ServerSocket == s) {
    this_sock = ServerSocket;
  } else if ((int)PeerSocket == s) {
    this_sock = PeerSocket;
  }

  // Read local ipv4 address
  sockaddr_in loc_addr = {};
  socklen_t name_len = sizeof(loc_addr);
  int err = getsockname(this_sock, (struct sockaddr*)&loc_addr, &name_len);

  // Convert ipv4 addr to c-str
  char buf[80] = "";
  const char* p = inet_ntop(AF_INET, &loc_addr.sin_addr, buf, 80);
  buf[79] = '\0';

  // Verify good addr
  if (!p) {
    COLORS.printError("[Error] Failed to retrieve local IPv4 addr\n");
    return -1;
  }

  // Build clientAddrInfo
  my_local_addr = convertSockAddrToClientAddrInfo(loc_addr);
  // std::cout << "[TEMP] Local IPv4 saved as: " << my_local_addr.rep_str << std::endl;

  return 1;
}

int NetEngine::initPeerSocket() {
  // Ensure peer socket NOT initialized yet
  if (peer_sock > 0 || PeerSocket != INVALID_SOCKET) { peerDisconnect(); }

  // Verify my_local_addr has been set
  if (my_local_addr.addr == 0) {
    COLORS.printError("[Error] Attempted peer socket initialization without setting my_local_addr\n");
    return -1;
  } 

  // Get a UDP socket
  if ((PeerSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
    COLORS.printError("[Error] Failed to get socket fd for peer.\n");
    return (peer_sock = -1);
  }
  // Update peer_sock fd
  peer_sock = (int)PeerSocket;

  // Get current valid outbound local address
  struct sockaddr_in loc_addr = convertClientAddrInfoToSockAddr(my_local_addr);

  COLORS.printInColor("[Log] --Logging Socket Initialization--\n", COLORS.brt_magenta_fg);
  std::stringstream log_sock_init;
  log_sock_init << "My local addr: " << my_local_addr.rep_str << std::endl;
  log_sock_init << "In loc_addr sockaddr: " << convertSockAddrToClientAddrInfo(loc_addr).rep_str << std::endl;
  COLORS.printInColor(log_sock_init.str(), COLORS.brt_cyan_fg);

  // Values to set options with
  int iOptVal = 0;
  int iOptLen = sizeof(int);
  BOOL bOptVal = FALSE;
  int bOptLen = sizeof(BOOL);

  // Enable safe reuse of port
  // Missing SO_REUSEPORT - winsock doesn't have it, hopefully that's alright
  // SO_CONDITIONAL_ACCEPT is worth looking into
  // so is SO_RCVTIMEO & SO_SNDTIMEO
  bOptVal = TRUE;
  if (setsockopt(PeerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&bOptVal, bOptLen) == SOCKET_ERROR) {
    printWindowsError("setsockopt - SO_REUSEADDR");
    peerDisconnect();
    return -1;
  }

  if (getsockopt(PeerSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&iOptVal, &iOptLen) == SOCKET_ERROR) {
    printWindowsError("getsockopt for SO_REUSEADDR");
    peerDisconnect();
    return -1;
  } else {
    std::cout << "[TEMP] SO_REUSEADDR Value: " << iOptVal << std::endl;
  }

  // Set non-blocking
  u_long iMode = 1; // disable blocking
  if (ioctlsocket(PeerSocket, FIONBIO, &iMode) != NO_ERROR) {
    std::stringstream ss;
    printWindowsError("ioctlsocket");
    peerDisconnect();
    return -1;
  }

  // TEMP log
  if (iMode == 0) { COLORS.printError("[Log] Peer socket initialized with blocking ON\n"); } 
  else { COLORS.printSuccess("[Log] Peer socket initialized with blocking OFF\n"); }

  // Bind to the same local address as was used for server comms
  if (bind(PeerSocket, (SOCKADDR *)&loc_addr, sizeof(loc_addr)) != NO_ERROR) {
    printWindowsError("bind");
    peerDisconnect();
    return -1;
  }

  if (updateLocalAddress(peer_sock) < 0) {
    std::cout << "[Error] Failed to update local address while initializing peer socket\n";
    peerDisconnect();
    return -1;
  }
  std::cout << "[Log] Peer Socket after initPeerSocket(): " << peer_sock << std::endl;

  return peer_sock;
}

ssize_t NetEngine::sendPeerSetupPacket(PeerSetupPacket out_pkt, clientAddrInfo some_addr) {
  if (PeerSocket == INVALID_SOCKET) {
    printf("Unable to connect to peer!\n");
    return -1;
  }

  ssize_t pkt_size = PEER_SETUP_PACKET_SIZE;

  struct sockaddr_in peer_sockaddr = convertClientAddrInfoToSockAddr(some_addr);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = (ssize_t)sendto(
      PeerSocket, 
      out_pkt.packet_buf+total_bytes_sent, 
      pkt_size-total_bytes_sent, 
      0,
      (sockaddr *)&peer_sockaddr, 
      sizeof(peer_sockaddr));
    total_bytes_sent += bytes_sent;
    if (bytes_sent == SOCKET_ERROR) {
      peerDisconnect();
      if (ENABLE_NETCODE_ERROR) {
        COLORS.printError("[Error] Failed to send packet:\n");
        out_pkt.printContents();
        printWindowsError("send peer setup packet");
      }
      return bytes_sent;
    }
  }

  return total_bytes_sent;
}

std::pair<PeerSetupPacket,clientAddrInfo> NetEngine::recvPeerSetupPacket() {
  //TODO: delete this ig
  clientAddrInfo peer_addr_tmp(0, 0);  // = convertSockAddrToClientAddrInfo(peer_sock)

  if (PeerSocket == INVALID_SOCKET) {
    printf("[Error] Unable to connect to server!\n");
    WSACleanup();
    return std::pair<PeerSetupPacket,clientAddrInfo>(PeerSetupPacket(), peer_addr_tmp);
  }

  char buf[PEER_SETUP_PACKET_SIZE] = "";

  struct sockaddr_in peer_sockaddr = {};
  int sockaddr_len = (int) sizeof(peer_sockaddr);

  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0, pkt_size = PEER_SETUP_PACKET_SIZE;
  while (total_bytes_in < pkt_size) {
    bytes_in = (ssize_t)recvfrom(
      PeerSocket, 
      buf+total_bytes_in, 
      pkt_size-total_bytes_in, 
      0,
      (sockaddr *)&peer_sockaddr,
      &sockaddr_len);
    total_bytes_in += bytes_in;
    if (bytes_in == SOCKET_ERROR) {
      // COLORS.printError("[Error] Failed to recv packet\n");
      // printWindowsError("recv peer setup packet");
      // peerDisconnect();
      return std::pair<PeerSetupPacket,clientAddrInfo>(PeerSetupPacket(), peer_addr_tmp);
    }
    // This may be unnecessary
    if (bytes_in == 0 && total_bytes_in != pkt_size) {
      COLORS.printError("[Error] Received malformed packet from peer.\n");
      return std::pair<PeerSetupPacket,clientAddrInfo>(PeerSetupPacket(), peer_addr_tmp);
    }
  }

  // Convert to ClientPacket
  PeerSetupPacket in_pkt(buf, pkt_size);
  peer_addr_tmp = convertSockAddrToClientAddrInfo(peer_sockaddr);
  return std::pair<PeerSetupPacket,clientAddrInfo>(in_pkt, peer_addr_tmp);
}

void NetEngine::peerDisconnect() {
  if (PeerSocket != INVALID_SOCKET) {
    // cleanup socket
    shutdown(PeerSocket, SD_BOTH);
    closesocket(PeerSocket);
    PeerSocket = INVALID_SOCKET;
  }
  peer_sock = -1; 
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
    serverDisconnect();
    return -1;
  }

  // // Enable safe reuse of port
  // int opt = 1;
  // if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0) {
  //   perror("[Error] setsockopt\n");
  //   serverDisconnect();
  //   return -1;
  // }

  // Connect to the server
  if (connect(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    COLORS.printError("connect() failed in serverConnect()\n");
    serverDisconnect();
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
    bytes_sent = send(server_sock, buf+total_bytes_sent, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
        COLORS.printError("[Error] Failed to send packet:\n");
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
    bytes_in = recv(s, buf+total_bytes_in, SERVER_PACKET_N_BYTES-total_bytes_in, 0);
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
  struct sockaddr_in loc_addr = {};
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
  // Ensure fresh peer socket
  if (peer_sock > 0) { 
    COLORS.printWarning("[Warning] Attempted peer socket initialization when peer socket already set\n");
    peerDisconnect(); 
  }

  // Verify my_local_addr has been set
  if (my_local_addr.addr == 0) {
    COLORS.printError("[Error] Attempted peer socket initialization without setting my_local_addr\n");
    return -1;
  } 

  // Get a UDP socket
  if ((peer_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    COLORS.printError("[Error] Failed to get socket fd for peer.\n");
    peerDisconnect();
    return -1;
  }

  // Enable safe reuse of port
  int opt = 1;
  if (setsockopt(peer_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    COLORS.printError("[Error] Failed to set peer socket options: ");
    perror("");
    peerDisconnect();
    return -1;
  }

  // Set non-blocking
  if (fcntl(peer_sock, F_SETFL, O_NONBLOCK) < 0) {
    COLORS.printError("[Error] Failed to set peer socket non-blocking: ");
    perror("");
    peerDisconnect();
    return -1;
  }

  struct sockaddr_in loc_addr = convertClientAddrInfoToSockAddr(my_local_addr);

  // Bind to the same local address as was used for server comms
  if (bind(peer_sock, (struct sockaddr*)&loc_addr, sizeof(loc_addr)) < 0) {
    COLORS.printError("[Error] Failed to bind");
    perror("");
    peerDisconnect();
    return -1;
  }

  if (updateLocalAddress(peer_sock) < 0) {
    COLORS.printError("[Error] Failed update local address via peer socket\n");
    peerDisconnect();
    return -1;
  }

  std::stringstream ss;
  ss << "[TEMP] Peer Socket fd: " << peer_sock << "@ " << my_local_addr.rep_str << std::endl;
  COLORS.printInColor(ss.str(), COLORS.brt_cyan_fg);

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

  struct sockaddr_in peer_sockaddr = {};
  socklen_t sockaddr_len = sizeof(peer_sockaddr);

  ssize_t bytes_in = 0, total_bytes_in = 0, pkt_size = PEER_SETUP_PACKET_SIZE;
  // Ensure full packet is read
  while (total_bytes_in < pkt_size) {
    bytes_in = recvfrom(
      peer_sock, // fd
      in_buf+total_bytes_in,  // buffer
      pkt_size-total_bytes_in, // bytes to send
      0, // flags
      (struct sockaddr*)&peer_sockaddr, // sockaddr
      &sockaddr_len // socklen
    );
    total_bytes_in += bytes_in;
    if (ENABLE_DENSE_PACKET_INSPECTION) {
      std::cout << "[DENSE PACKET - RECV] Bytes received: " << total_bytes_in << std::endl;
      std::cout << "[DENSE PACKET - RECV] Addr Len: " << sockaddr_len << std::endl;
    }
    if (bytes_in < 0) {
      // if (ENABLE_NETCODE_ERROR) {
      //   COLORS.printError("[Error] Failed to receive peer setup packet: ");
      //   perror("");
      // }
      return std::pair<PeerSetupPacket,clientAddrInfo>(PeerSetupPacket(),clientAddrInfo(0,0));
    }
  }

  // Convert to ClientPacket
  PeerSetupPacket in_pkt(in_buf, pkt_size);
  clientAddrInfo peer_addr_tmp = convertSockAddrToClientAddrInfo(peer_sockaddr);
  return std::pair<PeerSetupPacket,clientAddrInfo>(in_pkt,peer_addr_tmp);
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
    crossPlatformSleep(100);
    if (!continue_program) { exit(EXIT_FAILURE); }

    std::cout << "[Error] Invalid username: " << usr_name << std::endl;
    std::cout << "[Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid.)" << std::endl;
    // COLORS.printError(ss.str());

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
    crossPlatformSleep(100);
    if (!continue_program) { exit(EXIT_FAILURE); }
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

  // Update local addr info used with server
  if (updateLocalAddress(server_sock) < 0) {
    COLORS.printError("[Error] Update local addr failed\n");
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
  
  // Update local addr info used with server
  if (updateLocalAddress(server_sock) < 0) {
    COLORS.printError("[Error] Update local addr failed\n");
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

ssize_t NetEngine::sendPeerSetupBoth(PeerSetupPacket out_pkt) {
  // Check if final addr found -> only send there
  if (peer_addr_final.addr != 0) {
    // Initial first send, expected to be dropped
    if (sendPeerSetupPacket(out_pkt, peer_addr_final) < 0) {
      // UDP send should never fail
      COLORS.printError("[Error] Failed to send peer setup packet to FINAL addr.\n");
      peerDisconnect();
      return -1;
    } else {
      if (ENABLE_PEERPACKET_INSPECTION) {
        std::cout << "[Log] Sent peer with established = " 
        << (out_pkt.connection_established ? "true" : "false") 
        << " (FINAL = " << peer_addr_public.rep_str << ")" << std::endl;
      }
    }
    return 1;
  }

  // Final addr NOT found, send to both

  // Initial first send, expected to be dropped
  if (sendPeerSetupPacket(out_pkt, peer_addr_public) < 0) {
    // UDP send should never fail
    COLORS.printError("[Error] Failed to send peer setup packet to PUB addr.\n");
    peerDisconnect();
    return -1;
  } else {
    if (ENABLE_PEERPACKET_INSPECTION) {
      std::cout << "[Log] Sent peer with established = " 
      << (out_pkt.connection_established ? "true" : "false") 
      << " (PUB = " << peer_addr_public.rep_str << ")" << std::endl;
    }
  }
  // Attempt send to priv
  if (sendPeerSetupPacket(out_pkt, peer_addr_private) < 0) {
    // UDP send should never fail
    COLORS.printError("[Error] Failed to send peer setup packet to priv addr.\n");
    peerDisconnect();
    return -1;
  } else {
    if (ENABLE_PEERPACKET_INSPECTION) {
      std::cout << "[Log] Sent peer with established = " 
      << (out_pkt.connection_established ? "true" : "false") 
      << " (PRIV = " << peer_addr_private.rep_str << ")" << std::endl;
    }
  }
  return 1;
}

PeerSetupPacket NetEngine::initializePeerCommunication(uint16_t game_dur_f, uint8_t character_id) {
  if (initPeerSocket() <= 0) {
    COLORS.printError("[Error] Peer socket initialization failed while trying to init peer comms\n");
    return PeerSetupPacket();
  }
  peer_addr_final = clientAddrInfo(0,0);
  peer_connection_established = false;

  // Packet for initializing peer communication
  PeerSetupPacket out_pkt(false, game_dur_f, character_id, username); // Sent BEFORE connected

  PeerSetupPacket in_pkt = PeerSetupPacket();      // Current packet received
  PeerSetupPacket in_pkt_good; // Save the packet from peer marked 'connection_established'

  COLORS.printInColor("[Log] --TESTING P2P INITIALIZATION--\n", COLORS.brt_magenta_fg);
  std::stringstream addr_log;
  addr_log << " [LOC PRIV ADDR]: " << my_local_addr.rep_str << std::endl;
  addr_log << " [RMT PRIV ADDR]: " << peer_addr_private.rep_str << std::endl;
  addr_log << " [RMT PUB  ADDR]: " << peer_addr_public.rep_str << std::endl;
  COLORS.printInColor(addr_log.str(), COLORS.brt_cyan_fg);

  // First outward packet to trick NAT into thinking we initialized
  sendPeerSetupBoth(out_pkt);
  // 3s delay so we should have received theirs by now
  crossPlatformSleep(750);
  // send again - this one should go through
  sendPeerSetupBoth(out_pkt);

  // TODO: Check potential issue with bind interfering with message receipt? Bind to public?
  size_t n_attempts = 0, max_attempts = 30;
  // Switch between attempting connections with public & private addrs
  while (n_attempts < max_attempts && !peer_connection_established) {
    std::cout << std::endl; // May need to delete this (TEMP)
    n_attempts++;

    // Read from the socket
    std::pair<PeerSetupPacket, clientAddrInfo> received_info = recvPeerSetupPacket();
    // Parse received packet & addr
    in_pkt = received_info.first;
    clientAddrInfo tmp_recvd_addr = received_info.second;

    // Check if anything came through
    if (tmp_recvd_addr.addr == 0) { 
      COLORS.printWarning("[Warn] Packet not yet received...\n");
      // Nothing came through -> send again
      sendPeerSetupBoth(out_pkt);
      // Wait half a second before checking again
      crossPlatformSleep(250);
      continue; 
    }

    // Check address of incoming packet
    if (tmp_recvd_addr.addr == peer_addr_public.addr) {
      // Address matches peer public
      peer_addr_final = peer_addr_public; // Addr matches, set as final addr
      std::stringstream ss;
      ss << "[Log] Received packet from peer public addr: " << tmp_recvd_addr.rep_str << "\n";
      COLORS.printSuccess(ss.str());
      in_pkt.printContents();
    } else if (tmp_recvd_addr.addr == peer_addr_private.addr) {
      // Address matches peer private
      peer_addr_final = peer_addr_private; // Addr matches, set as final addr
      std::stringstream ss;
      ss << "[Log] Received packet from peer private addr: " << tmp_recvd_addr.rep_str << "\n";
      COLORS.printSuccess(ss.str());
      in_pkt.printContents();
    } else {
      // Address did not match either expected, begone
      COLORS.printWarning("[Warn] Received packet from unknown address, discarding\n");
      continue;
    }

    // Check contents of incoming packet
    if ((std::string)in_pkt.user_name == "") {
      COLORS.printWarning("[Warn] Peer initialization packet received containing invalid username.\n");
      continue;
    } 

    // Only reaches here if received a good packet
    out_pkt = PeerSetupPacket(true, game_dur_f, character_id, username);
    sendPeerSetupBoth(out_pkt);

    // Notify when peer has received our packets
    if (in_pkt.connection_established) {
      if (peer_connection_established) { 
        COLORS.printSuccess("[Log] Peer handshake finished\n");
        break; 
      }
      // May need to clear socket fd here for fresh packets going forward
      COLORS.printSuccess("[Log] Peer has received our packets!\n");
      peer_connection_established = true;
    }
  }
  if (peer_addr_final.addr == 0) { 
    COLORS.printWarning("[Warn] Failed to initialize peer communication.\n");
  }
  return in_pkt; 
}

int NetEngine::testNetClient() {
  COLORS.printInColor("\n[Log] Running network test...\n", COLORS.brt_magenta_fg);
  Timer timer;
  timer.start();
  ssize_t result;

  // std::cout << "[TEMP] Testing local addr record & save\n";
  // serverConnect();
  // updateLocalAddress(server_sock);
  // serverDisconnect();
  // std::stringstream ss;
  // ss << "\nRetrieved local addr as: " << my_local_addr.rep_str << std::endl;
  // COLORS.printSuccess(ss.str());
  // return -1;

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
      crossPlatformSleep(100);
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
    if (!continue_program) { exit(EXIT_FAILURE); }
    std::cout << "\n[Log] Requesting peer addr (" << timer.duration() << "s)" << std::endl;

    // Check if someone has connected to the server
    getPeerAddr();

    // Update our condition
    bad_addrs = (peer_addr_public.addr == 0 && peer_addr_private.addr == 0);
    if (!bad_addrs) { break; }

    // Repeatedly ask the server, waiting 1s in-between
    crossPlatformSleep(500);
  }

  // Display success statement
  COLORS.printSuccess("\nSuccessfully got peer addr from server!\n");

  // Initiate p2p (udp holepunch - https://bford.info/pub/net/p2pnat/)

  // Connect to peer and test communication
  std::cout << "\n[Log] Initializing peer communication\n";
  initializePeerCommunication((uint16_t)(60*60*5), CHARACTER_ID_HUNKO);

  std::cout << "[TEMP] Peer socket fd: " << peer_sock << std::endl;
  std::cout << "[TEMP] Local addr: " << my_local_addr.rep_str << std::endl;
  std::cout << "[TEMP] Peer addr final: " << peer_addr_final.rep_str << std::endl;

  // Close peer connection
  peerDisconnect();

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}
