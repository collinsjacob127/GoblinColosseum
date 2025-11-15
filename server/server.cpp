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

  // Comparison times for clean printing
  uint64_t cur_time_hs = (uint64_t)std::floor(10 * server_timer.duration());
  uint64_t prev_time_hs = cur_time_hs;

  // Track lobby size
  size_t n_clients = 0;

  // Good cout formatting
  std::cout << std::fixed << std::setprecision(1) << std::endl;

  // Flow control
  bool continue_server = true;
  // Main loop of server running
  while (continue_server) {
    // Server automatic shutoff
    cur_time_hs = std::floor(10 * server_timer.duration());
    if (ENABLE_AWAITING_NEW_PACKETS_NOTIF && cur_time_hs != prev_time_hs) {
      prev_time_hs = cur_time_hs;
      std::cout << std::flush;
      std::cout << ANSI_ESCAPES.carriage_return;
      std::cout << ANSI_ESCAPES.brt_magenta_fg;
      std::cout << "[Server Awaiting New Messages - Runtime: "
      << server_timer.duration() << "s]";
    }

    //TODO: Remove inactive players after 10 minutes
    
    //TODO: Remove inactive lobbies after 15 minutes

    // Bind socket to local interface and passive open
    struct sockaddr_in new_address;
    socklen_t addr_len = sizeof(new_address);
    if ((client_socket = accept(listen_socket, (struct sockaddr*)&new_address, &addr_len)) < 0) {
      continue;
    } else {
      std::cout << "\n";
      std::cout << ANSI_ESCAPES.white_fg; // White
      // Accept succeeded
      if (ENABLE_SERVER_DEBUG) {
        std::cout << "[Debug] Client connected via socket " << client_socket 
        << " on port " << ntohs(new_address.sin_port) << std::endl; 
      }
    }

    // Guarantee server notif message shows between all handlings
    prev_time_hs = 0;

    // Receive client's packet
    ClientPacket in_pkt = recvClientPacket(CLIENT_PACKET_N_BYTES, client_socket);

    // Respond to the packet
    int response = -1;
    switch (in_pkt.packet_type) {
      case (10): {
        response = 969; // Empty packet, ignore
        break;
      }
      case (0): {
        response = initializePlayer(in_pkt, client_socket);
        break;
      }
      case (1): {
        response = createLobby(in_pkt, client_socket, new_address);
        break;
      }
      case (2): {
        response = sendLobbies(in_pkt, client_socket);
        break;
      }
      case (3): {
        response = joinLobby(in_pkt, client_socket, new_address);
        break;
      }
      case (4): {
        response = sendPeerInfo(in_pkt, client_socket);
        break;
      }
      default: {
        if (ENABLE_SERVER_ERROR) {
          ANSI_ESCAPES.printError("[Error] Invalid packet type received.\n");
        }
      }
    }

    if (response < 0 && ENABLE_SERVER_ERROR) {
      ANSI_ESCAPES.printError("[Error] Server response indicated some failure.\n");
    }

    // Close the connection
    close(client_socket);

    if (n_clients != registry.size()) {
      n_clients = registry.size();
      std::cout << ANSI_ESCAPES.cyan_fg;
      std::cout << "[The registry has " << n_clients << " entries.]" << std::endl;
      std::cout << ANSI_ESCAPES.white_fg;
    }
    if (response != 969) {
      std::cout << std::endl;
    }
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
        ss << "[Error] Failed to receive packet from socket %d" << s << std::endl;
        ANSI_ESCAPES.printInColor(ss.str(), ANSI_ESCAPES.red_fg);
        perror("");
      }
      return ClientPacket(0, 0, 0, "");
    }
    if (total_bytes_in == 0) {
      return ClientPacket(10, 0, 0, "");
    }
    if (total_bytes_in < n_bytes && bytes_in == 0) {
      ANSI_ESCAPES.printError("[Error] Received packet of incorrect size\n");
      return ClientPacket(69, 0, 0, "");
    }
  }

  // Convert to ClientPacket
  ClientPacket out_pkt(buf, n_bytes);
  return out_pkt;
}

ssize_t sendServerPacket(ServerPacket out_pkt, int s) {
  unsigned char buf[SERVER_PACKET_N_BYTES];

  // Populate buffer with packet contents (prepped for netsend)
  ssize_t pkt_size = out_pkt.buildPacket(buf);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(s, buf, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_SERVER_ERROR) {
        ANSI_ESCAPES.printInColor("[Error] Failed to send packet.\n", ANSI_ESCAPES.red_fg);
        perror("");
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
  uint64_t session_id = registry.addPlayer(user_name);

  // Send session ID back to player
  ServerPacket out_pkt(in_pkt.packet_type, session_id, 0, "Session ID Sent");
  if (sendServerPacket(out_pkt, client_sock) < 0 || bad_username) {
    ANSI_ESCAPES.printInColor("[Error] Failed to initialize client\n", ANSI_ESCAPES.red_fg);
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

int createLobby(ClientPacket in_pkt, int client_sock, sockaddr_in client_addr) {
  PlayerEntry *cur_player;
  uint64_t session_id = in_pkt.session_id, lobby_id;

  // Get and verify current user
  if (!(cur_player =registry.getPlayer(session_id, SESSION_ID_SPECIFIER))) {
    // User does not exist
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "Invalid session ID");
    ANSI_ESCAPES.printError("[Error] Invalid session ID\n");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Player doesn't exist in registry
  }

  // Set new lobby ID
  lobby_id = registry.setNewId(cur_player, LOBBY_ID_SPECIFIER);

  // Send lobby ID to player
  ServerPacket out_pkt(in_pkt.packet_type, session_id, lobby_id, "Lobby created");
  if (sendServerPacket(out_pkt, client_sock) < 0) {
    registry.clearId(lobby_id, LOBBY_ID_SPECIFIER);
    ANSI_ESCAPES.printError("[Error] Response failed to send\n");
    return -1; 
  }

  // Build private IP struct from received contents
  in_pkt.contents[CLIENT_CONTENTS_SIZE-1] = '\0';
  clientAddrInfo player_addr_priv(in_pkt.contents);
  clientAddrInfo player_addr_pub(client_addr.sin_addr.s_addr, client_addr.sin_port);

  // Set addresses for this player (to be sent via sendPeerInfo)
  cur_player->player_addr_private = player_addr_priv;
  cur_player->player_addr_public = player_addr_pub;

  // Print Public & Private IPs
  if (ENABLE_SERVER_LOG) {
    std::stringstream ss;
    ss << "[Log] Client \"" << cur_player->user_name
    << "\" successfully created lobby with id: " << cur_player->lobby_id << std::endl;
    ANSI_ESCAPES.printSuccess(ss.str());
    ss << std::flush;
    std::cout << "[Log] Address of player \"" << cur_player->user_name << "\" marked as:\n";
    std::cout << " [PUB  ADDR] " << cur_player->player_addr_public.rep_str << std::endl;
    std::cout << " [PRIV ADDR] " << cur_player->player_addr_private.rep_str << std::endl;
  }


  // Start the lobby update timer
  cur_player->lobby_update_time.start();

  return 1;
}

int sendLobbies(ClientPacket in_pkt, int client_sock) {
  // This request parses client packet differently
  size_t min_idx = (size_t) in_pkt.session_id;
  size_t max_idx = (size_t) in_pkt.lobby_id;
  size_t n_requested_lobbies = max_idx - min_idx;

  // Verify valid inputs
  if (n_requested_lobbies > MAX_N_REQUESTED_LOBBIES) {
    ANSI_ESCAPES.printInColor( "[Error] User lobby index request mismatch\n", ANSI_ESCAPES.red_fg);
    sendServerPacket(ServerPacket(in_pkt.packet_type, 0, 0, ""), client_sock);
  }

  // Generate lobby list
  std::vector<TYPE_LOBBY_INFO> lobby_list = registry.getLobbyList(min_idx, max_idx);

  if (ENABLE_SERVER_DEBUG) { std::cout << "[Debug] Building lobby list packet\n"; }

  // Populate lobby list packet
  ServerPacket out_pkt(in_pkt.packet_type, lobby_list.size(), registry.getNumLobbies(), lobby_list);

  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Lobby id range: " << (uint64_t)min_idx 
    << " " << (uint64_t)max_idx << std::endl;
    std::cout << "[Debug] # Lobbies sent: " 
    << (uint64_t)lobby_list.size() << std::endl;
  }

  if (sendServerPacket(out_pkt, client_sock) < 0) {
    // registry.clearId(lobby_id, LOBBY_ID_SPECIFIER);
    return -1; 
  }

  return 1;
}

int joinLobby(ClientPacket in_pkt, int client_sock, sockaddr_in client_addr) {
  PlayerEntry *cur_player;
  uint64_t session_id = in_pkt.session_id, lobby_id = in_pkt.lobby_id;
  // Get and verify current user
  if (!(cur_player =registry.getPlayer(session_id, SESSION_ID_SPECIFIER))) {
    // User does not exist
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "Invalid session ID");
    ANSI_ESCAPES.printError("[Error] Invalid session ID\n");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Player doesn't exist in registry
  }

  if (ENABLE_SERVER_LOG) {
    std::cout << "[Log] Player \"" << cur_player->user_name
    << "\" requesting to join lobby " << lobby_id << std::endl;
  }

  // Get and verify lobby
  PlayerEntry *lobby_owner;
  if (!(lobby_owner = registry.getPlayer(lobby_id, LOBBY_ID_SPECIFIER))) {
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "Invalid lobby ID");
    ANSI_ESCAPES.printError("[Error] Invalid lobby ID\n");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Lobby DNE
  }
  if (lobby_owner->match_made) {
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "Lobby already full.");
    ANSI_ESCAPES.printError("[Error] Lobby already full\n");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Lobby full
  }

  // Send lobby ID to player
  ServerPacket out_pkt(in_pkt.packet_type, session_id, lobby_id, "Lobby joined");
  if (sendServerPacket(out_pkt, client_sock) < 0) {
    ANSI_ESCAPES.printError("[Error] Response failed to send\n");
    registry.clearId(lobby_id, LOBBY_ID_SPECIFIER);
    return -1; 
  }

  // Mark players with match_made
  lobby_owner->match_made = true;
  cur_player->match_made = true;

  // Mark players with eachothers' session IDs
  lobby_owner->peer_session_id = cur_player->session_id;
  cur_player->peer_session_id = lobby_owner->session_id;

  // Build private IP struct from received contents
  in_pkt.contents[CLIENT_CONTENTS_SIZE-1] = '\0';
  clientAddrInfo player_addr_priv(in_pkt.contents);
  clientAddrInfo player_addr_pub(client_addr.sin_addr.s_addr, client_addr.sin_port);

  // Set addresses for this player (to be sent via sendPeerInfo)
  cur_player->player_addr_private = player_addr_priv;
  cur_player->player_addr_public = player_addr_pub;

  if (ENABLE_SERVER_LOG) {
    std::stringstream ss;
    ss << "[Log] Client \"" << cur_player->user_name
    << "\" successfully joined lobby \"" << lobby_owner->user_name << "\"\n";
    ANSI_ESCAPES.printSuccess(ss.str());
    ss << std::flush;
    std::cout << "[Log] Address of player \"" << cur_player->user_name << "\" marked as:\n";
    std::cout << " [PUB  ADDR] " << cur_player->player_addr_public.rep_str << std::endl;
    std::cout << " [PRIV ADDR] " << cur_player->player_addr_private.rep_str << std::endl;
  }

  // Start the lobby update timer
  cur_player->lobby_update_time.start();
  lobby_owner->lobby_update_time.start();

  return 1;
}

int sendPeerInfo(ClientPacket in_pkt, int client_sock) {
  PlayerEntry *cur_player;
  uint64_t session_id = in_pkt.session_id, lobby_id = in_pkt.lobby_id;

  // Get and verify current user
  if (!(cur_player = registry.getPlayer(session_id, SESSION_ID_SPECIFIER))) {
    // User does not exist
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    ANSI_ESCAPES.printInColor("[Error] Client not in registry\n", ANSI_ESCAPES.red_fg);
    return -1; // Player doesn't exist in registry
  }

  if (ENABLE_SERVER_LOG) {
    std::cout << "[Log] Client \"" << cur_player->user_name << "\" requests peer addr\n";
  }

  // Get and verify lobby
  PlayerEntry *lobby_owner;
  if (!(lobby_owner = registry.getPlayer(lobby_id, LOBBY_ID_SPECIFIER))) {
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    ANSI_ESCAPES.printInColor("[Error] Requested lobby DNE\n", ANSI_ESCAPES.red_fg);
    return -1; // Lobby DNE
  }
  if (!lobby_owner->match_made || !cur_player->match_made) {
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    ANSI_ESCAPES.printInColor("[Error] Match has not yet been made.\n", ANSI_ESCAPES.yellow_fg);
    return 1; // Match not made
  }

  // Get and verify peer
  PlayerEntry *cur_peer;
  if (!(cur_peer = registry.getPlayer(cur_player->peer_session_id, SESSION_ID_SPECIFIER))) {
    // Invalid peer
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    ANSI_ESCAPES.printInColor("[Error] Peer requested by client DNE\n", ANSI_ESCAPES.red_fg);
    return -1; // Peer DNE
  }

  // Verify VALID match
  if (cur_peer->peer_session_id != cur_player->session_id
    || cur_player->peer_session_id != cur_peer->session_id
    ) {
    ANSI_ESCAPES.printInColor("[Error] Matchmaking peer session ID mismatch\n" , ANSI_ESCAPES.red_fg);
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Match not made
  }

  // Verify Addrs have been set
  if (cur_player->player_addr_private.addr == 0
      || cur_player->player_addr_public.addr == 0
      || cur_peer->player_addr_public.addr == 0
      || cur_peer->player_addr_private.addr == 0) {
    ANSI_ESCAPES.printInColor("[Error] Players addrs not yet set\n" , ANSI_ESCAPES.red_fg);
    ServerPacket out_pkt(in_pkt.packet_type, 0, 0, "");
    sendServerPacket(out_pkt, client_sock);
    return -1; // Match not made
  }

  if (ENABLE_SERVER_LOG) {
    std::cout << "[Log] Sending addr of " << cur_peer->user_name
    << " to " << cur_player->user_name << std::endl;
  }

  // Send peer addrs to client
  ServerPacket out_pkt(in_pkt.packet_type, session_id, lobby_id, 
    cur_peer->player_addr_public, cur_peer->player_addr_private);

  if (sendServerPacket(out_pkt, client_sock) < 0) {
    registry.clearId(lobby_id, LOBBY_ID_SPECIFIER);
    ANSI_ESCAPES.printInColor("[Error] Failed to send packet to client\n", ANSI_ESCAPES.red_fg);
    return -1; 
  }

  if (ENABLE_SERVER_LOG) {
    std::stringstream ss; 
    ss << "[Log] Successfully sent peer addrs to ";
    ss << cur_player->user_name << std::endl;
    ANSI_ESCAPES.printSuccess(ss.str());
    std::cout << ANSI_ESCAPES.cyan_fg;
    std::cout << " [PUB  ADDR] " << cur_player->player_addr_public.rep_str << std::endl;
    std::cout << " [PRIV ADDR] " << cur_player->player_addr_private.rep_str << std::endl;
    std::cout << ANSI_ESCAPES.white_fg << std::flush;
  }

  // Check if finished
  if (cur_player->first_addr_sent || cur_peer->first_addr_sent) {
    // One address has already been successfuly sent. This was the last one.

    // Delete players from registry
    std::cout << "[Log] Removing players from registry" << std::endl;
    registry.removePlayer(cur_player);
    registry.removePlayer(cur_peer);
    return 1;
  }

  // Set in registry of both players that the first addr has been sent
  cur_player->first_addr_sent = true;
  cur_peer->first_addr_sent = true;

  // Start the lobby update timer
  cur_player->lobby_update_time.start();
  lobby_owner->lobby_update_time.start();

  return 1;
}

int bindAndListen(const char *service) {
  struct addrinfo hints;
  struct addrinfo *rp, *result;
  int s;

  /* Build address data structure */
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
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

    // Set non-blocking
    if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
      perror("fcntl");
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
  printf("[Log] User on socket %d disconnected.\n", fd);
}

void handleSigint(int signal_num) {
  // Print interruption message
  std::cout << std::flush << std::endl << ANSI_ESCAPES.yellow_fg << std::flush;
  fprintf(stderr, "\n[WARN] Server Interrupted (Received signal %d)\n", signal_num);

  std::cout << ANSI_ESCAPES.green_fg;
  std::cout << "[Log] Disconnecting all clients and shutting down server.\n";

  // Close all sockets
  closeAllConnections();

  std::cout << "[Log] Server shutting down safely.\n";
  std::cout << "[Log] Removing " << registry.size()
  << " entries from lobby." << ANSI_ESCAPES.white_fg << std::endl;
  exit(EXIT_SUCCESS);
}
