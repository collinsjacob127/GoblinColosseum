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
int listen_socket, client_socket;

static Registry registry;

int main() {
  // Safely clean up the server if shit breaks.
  std::signal(SIGINT, handleSigint);
  std::signal(SIGABRT, handleSigint);
  std::signal(SIGTERM, handleSigint);

  // Timer to track how long the server has been up
  Timer server_timer;

  // Socket where server accept() new connections thru
  listen_socket = bindAndListen(SERVER_PORT);
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Server listening on port " 
    << SERVER_PORT << " via socket " << listen_socket << std::endl;
  }

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

    // Bind socket to local interface and passive open
    struct sockaddr_in new_address;
    socklen_t addr_len = sizeof(new_address);
    if ((client_socket = accept(listen_socket, (struct sockaddr*)&new_address, &addr_len)) < 0) {
      // Accept failed
      perror("[Error] Invalid accept attempted, closing server and exiting");
      closeAllConnections();
      exit(EXIT_FAILURE);
    } else {
      // Accept succeeded
      if (ENABLE_SERVER_DEBUG) {
        std::cout << "[Debug] Client connected via socket " << client_socket << std::endl; 
      }
    }

    // Receive client's packet
    ClientPacket in_pkt = recvClientPacket(CLIENT_PACKET_N_BYTES, client_socket);
    if (ENABLE_SERVER_DEBUG) {
      std::cout << "[Debug] Packet received: \n" << in_pkt.getStringFromSelf();
    }

    // Respond to the packet
    int response = -1;
    switch (in_pkt.packet_type) {
      case (0): {
        response = initializePlayer(in_pkt, client_socket);
        break;
      }
      case (1): {
        response = createLobby(in_pkt, client_socket);
        break;
      }
      case (2): {
        response = sendLobbies(in_pkt, client_socket);
        break;
      }
      case (3): {
        response = sendPeerInfo(in_pkt, client_socket);
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
    close(client_socket);
  }

  // Close the server's socket
  closeAllConnections();
  printf("[Log] Server closing...\n");
  return 0;
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
  // Verify good username
  std::string user_name = in_pkt.contents;
  bool bad_username = false;
  if (user_name.length() == 0 || user_name.length() >= MAX_USERNAME_SIZE) {
    bad_username = true;
  }

  // Add player to registry and get session ID
  uint64_t session_id = registry.addPlayer();

  // Send session ID back to player
  ServerPacket out_pkt(in_pkt.packet_type, session_id, 0, "Session ID Sent");
  if (sendServerPacket(out_pkt, client_sock) < 0 || bad_username) {
    // Failed to send, remove them from the session
    registry.clearId(session_id, SESSION_ID_SPECIFIER);
    return -1; 
  }

  // Set username
  PlayerEntry *new_player = registry.getPlayer(session_id, SESSION_ID_SPECIFIER);
  new_player->user_name = in_pkt.contents;

  // Start their timer
  new_player->p_timer.start();

  return 1;
}

int createLobby(ClientPacket in_pkt, int client_sock) {
  PlayerEntry *cur_player;
  uint64_t session_id = in_pkt.session_id, lobby_id;

  // Get and verify current user
  if (!(cur_player =registry.getPlayer(session_id, SESSION_ID_SPECIFIER))) {
    // User does not exist
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "Player DNE");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Player doesn't exist in registry
  }

  // Set new lobby ID
  lobby_id = registry.setNewId(cur_player, LOBBY_ID_SPECIFIER);

  // Send lobby ID to player
  ServerPacket out_pkt(in_pkt.packet_type, session_id, lobby_id, "Lobby created");
  if (sendServerPacket(out_pkt, client_sock) < 0) {
    registry.clearId(lobby_id, LOBBY_ID_SPECIFIER);
    return -1; 
  }

  // Start the lobby update timer
  cur_player->lobby_update_time.start();

  return 1;
}

int sendLobbies(ClientPacket in_pkt, int client_sock) {
  return -1;
}

int sendPeerInfo(ClientPacket in_pkt, int client_sock) {
  return -1;
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

    int opt = 1;
    // Enable safe reuse of server port
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
      perror("setsockopt");
      close(listen_socket);
      exit(EXIT_FAILURE);
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

void closeAllConnections() {
  close(listen_socket);
  close(client_socket);
}

void disconnectClient(int fd) {
  close(fd);
  printf("[Log] User on port %d disconnected.\n", fd);
}

void handleSigint(int signal_num) {
  fprintf(stderr, "\n\n[Log] Server Interrupted (Received signal %d)\n", signal_num);
  std::cout << "[Log] Disconnecting all clients and shutting down server.\n";
  // Close all sockets
  closeAllConnections();

  std::cout << "[Log] Server shutting down safely.\n";
  exit(EXIT_SUCCESS);
}
