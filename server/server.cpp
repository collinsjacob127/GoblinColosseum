/**
 * Author: Jacob Collins
 * 
 * Description:
 *  Server for matchmaking, and potentially other future server-side features.
 *  When requesting an online game, users send packets here, then this script
 *  connects users- sending them eachothers IPs, then waits until they time out
 *  or until a match_end notification packet arrives, at which point the servers
 *  records will be updated.
 * 
 * References:
 * https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 */

#include "server.hpp"

/**
 * GLOBALS for server; Necessary for cleaning up from SIGINT
 */
// all sockets -> all active
// call_set -> oft overwritten, only used for select()
static fd_set all_sockets, call_set;
static int listen_socket;
static int min_fd = 3, max_socket;

// Maps session_id -> PlayerEntry
static std::map<uint64_t, PlayerEntry> registry;

int main() {
  // Safely clean up the server if shit breaks.
  std::signal(SIGINT, handleSigint);
  std::signal(SIGABRT, handleSigint);
  std::signal(SIGTERM, handleSigint);

  // Timer to track how long the server has been up
  Timer server_timer;

  // Socket where server accept() new connections thru
  FD_ZERO(&all_sockets);
  listen_socket = bindAndListen(SERVER_PORT);
  int opt = 1;
  if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    close(listen_socket);
    exit(EXIT_FAILURE);
  }
  FD_SET(listen_socket, &all_sockets);
  // always equal to the max fd from which the program can accept() new conns
  max_socket = listen_socket;

  if (ENABLE_SERVER_DEBUG) { std::cout << "[Debug] Initial registry size: " << registry.size() << std::endl; }
  if (ENABLE_SERVER_LOG) { printf("[Log] Server has started!\n"); }

  // Main loop of server running
  bool continue_server = true;

  while (continue_server) {
    // Server automatic shutoff
    if (server_timer.duration() > 120.0) { continue_server = false; }

    if (ENABLE_AWAITING_NEW_PACKETS_NOTIF) {
      std::cout << "\n[Server Awaiting New Messages]\n";
    }

    int client_sock = -1;
    // Bind socket to local interface and passive open
    struct sockaddr_in new_address;
    socklen_t addr_len = sizeof(new_address);
    if ((client_sock = accept(listen_socket, (struct sockaddr*)&new_address, &addr_len)) < 0) {
      // Accept failed
      perror("[Error] Invalid accept attempted, closing server and exiting");
      closeAllInSet(&all_sockets, 3, max_socket);
      exit(EXIT_FAILURE);
    } else {
      // Accept succeeded
      FD_SET(client_sock, &all_sockets);
      max_socket = std::max(max_socket, client_sock);
      if (ENABLE_SERVER_DEBUG) {
        std::cout << "[Debug] Client connected via socket " << client_sock << std::endl; 
      }
    }

    // Receive client's packet
    ClientPacket in_pkt = recvClientPacket(CLIENT_PACKET_N_BYTES, client_sock);
    if (ENABLE_SERVER_DEBUG) {
      std::cout << "[Debug] Packet received: \n" << in_pkt.getStringFromSelf();
    }

    // Respond to the packet
    int response = -1;
    switch (in_pkt.packet_type) {
      case (0): {
        response = initializePlayer(in_pkt, client_sock);
        break;
      }
      case (1): {
        response = createLobby(in_pkt, client_sock);
        break;
      }
      case (2): {
        response = sendLobbies(in_pkt, client_sock);
        break;
      }
      case (3): {
        response = sendPeerInfo(in_pkt, client_sock);
        break;
      }
      default: {
        if (ENABLE_SERVER_ERROR) {
          std::cout << "[Error] Invalid packet type received.\n";
        }
      }
    }

    if (response < 0 && ENABLE_SERVER_ERROR) {
      std::cerr << "[Error] Server response failed.\n";
    }

    // Close the connection
    close(client_sock);
    FD_CLR(client_sock, &all_sockets);
  }

  // Close the server's socket
  closeAllInSet(&all_sockets, 3, max_socket);
  printf("[Log] Server closing...\n");
  return 0;
}

int bindAndListen(const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Build address data structure */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;

  /* Get local address info */
  if ((s = getaddrinfo(NULL, service, &hints, &result)) != 0) {
    fprintf(stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror(s));
    return -1;
  }

  /* Iterate through the address list and try to perform passive open */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1) {
      continue;
    }

    if (!bind(s, rp->ai_addr, rp->ai_addrlen)) {
      break;
    }

    close(s);
  }
  if (rp == NULL) {
    perror("stream-talk-server: bind");
    return -1;
  }
  if (listen(s, MAX_PENDING) == -1) {
    perror("stream-talk-server: listen");
    close(s);
    return -1;
  }
  freeaddrinfo(result);

  return s;
}

ClientPacket recvClientPacket(ssize_t n_bytes, int s) {
  unsigned char buf[BUFFER_SIZE];

  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Beginning full recv\n";
  }

  // Ensure full packet is read
  ssize_t bytes_in = 0, total_bytes_in = 0;
  while (total_bytes_in < n_bytes) {
    bytes_in = recv(s, buf, n_bytes-total_bytes_in, 0);
    total_bytes_in += bytes_in;
    if (bytes_in < 0) {
      if (ENABLE_SERVER_ERROR) {
        std::stringstream ss;
        ss << "[Error] Failed to receive packet from %d" << s << std::endl;
        perror(ss.str().c_str());
      }
      return ClientPacket(0, 0, 0, "");
    }
  }

  // Convert to ClientPacket
  ClientPacket out_pkt(buf, n_bytes);
  return out_pkt;
}

ssize_t sendServerPacket(ServerPacket out_pkt, int s) {
  unsigned char buf[BUFFER_SIZE];

  // Populate buffer with packet contents (prepped for netsend)
  ssize_t pkt_size = out_pkt.buildPacket(buf);

  ClientPacket test_pkt(buf, pkt_size);
  std::cout << "Test packet contents:\n" << test_pkt.getStringFromSelf();

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(s, buf, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_SERVER_ERROR) {
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

int initializePlayer(ClientPacket in_pkt, int client_sock) {
  uint64_t session_id = generateSessionId();
  ServerPacket out_pkt(in_pkt.packet_type, session_id, 0, "Session ID Sent");
  if (sendServerPacket(out_pkt, client_sock) < 0) {
    return -1; 
  }
  PlayerEntry new_player(session_id, in_pkt.contents);
  registry.insert_or_assign(session_id, new_player);
  return 1;
}

int createLobby(ClientPacket in_pkt, int client_sock) {
  // Get and verify current user
  PlayerEntry *cur_player;
  if (registry.find(in_pkt.session_id) != registry.end()) {
    cur_player = &registry.at(in_pkt.session_id);
  } else {
    return -1;
  }
  // Get & Set Session & Lobby IDs
  uint64_t session_id = cur_player->id;
  uint64_t lobby_id = generateLobbyId();
  ServerPacket out_pkt(in_pkt.packet_type, session_id, lobby_id, "Lobby created");
  // Send lobby ID to player
  if (sendServerPacket(out_pkt, client_sock) < 0) {
    return -1; 
  }
  // TODO: Set Lobby ID and mark as open lobby

  return 1;
}

int sendLobbies(ClientPacket in_pkt, int client_sock) {
  return -1;
}

int sendPeerInfo(ClientPacket in_pkt, int client_sock) {
  return -1;
}

uint64_t generateSessionId() {
  uint64_t rand_n = id_generator.getRandomId();

  // Guarantee unique session ID
  while (registry.find(rand_n) != registry.end()) {
    rand_n = id_generator.getRandomId();
  }

  return rand_n;
}

uint64_t generateLobbyId() {
  return 0;
}

void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd) {
  for (int i = min_fd; i <= max_fd; ++i) {
    // Ignore unset fds
    if (!FD_ISSET(i, socket_list))
      continue;
    
    close(i);
  }
}

PlayerEntry getEntryFromUserName(std::string uname) {
  PlayerEntry tmp;
  std::map<uint64_t, PlayerEntry>::iterator it;
  // Iterate through registry
  for (it = registry.begin(); it != registry.end(); it++) {
    // Check if user has matching name
    if (it->second.user_name == uname) {
      tmp = it->second;
    }
  }
  return tmp;
}

std::vector<std::string> getLobbyList() {
  std::vector<std::string> lobby_list;
  std::map<uint64_t, PlayerEntry>::iterator it;
  // Iterate through registry
  for (it = registry.begin(); it != registry.end(); it++) {
    // Check if user has open lobby
    if (it->second.open_lobby) {
      lobby_list.push_back(it->second.user_name);
    }
  }
  return lobby_list;
}

void removePlayer(uint64_t session_id) {
  printf("[Log] User %s (%lu) @ %s:%u has been removed from the registry.\n", 
    registry.at(session_id).user_name.c_str(),
    registry.at(session_id).id,
    registry.at(session_id).ipv4_str.c_str(),
    registry.at(session_id).port_num
  );
  std::map<uint64_t, PlayerEntry>::iterator it = registry.find(session_id);
  registry.erase(it);
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Server registry now has " << registry.size() << " entries\n";
  }
}

void disconnectClient(int fd) {
  close(fd);
  FD_CLR(fd, &all_sockets);
  printf("[Log] User on port %d disconnected.\n", fd);
}

void handleSigint(int signal_num) {
  std::cout << "\n[Log] Server Interrupted - Disconnecting all clients and shutting down server.\n";
  closeAllInSet(&all_sockets, min_fd, max_socket);
  FD_ZERO(&all_sockets);
  FD_ZERO(&call_set);
  std::cout << "[Log] Server shutting down safely.\n";
  exit(EXIT_SUCCESS);
}
