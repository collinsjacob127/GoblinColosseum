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
    // ...

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

void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd) {
  for (int i = min_fd; i <= max_fd; ++i) {
    // Ignore unset fds
    if (!FD_ISSET(i, socket_list))
      continue;
    
    close(i);
  }
}

/**
 * @brief Given the current registry, return the PlayerEntry of whichever
 * client has the corresponding username.
 * @return Returns the corrent player entry, or default player entry on not-found.
 */
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

ssize_t sendConfirmationResponse(int fd, bool is_good) {
  std::string response;
  if (is_good) {
    response = "GOOD";
  } else {
    response = "BAD";
  }

  ssize_t n_bytes;
  if ((n_bytes = send(fd, response.c_str(), sizeof(response), 0)) < 0) {
    if (ENABLE_SERVER_LOG) {
      std::cout << "[Log] Creation confirmation response successfuly sent (" 
      << response.c_str() << ")" << std::endl;
    }
  } else {
    if (ENABLE_SERVER_ERROR) {
      perror("[Error] Failed to send creation confirmation response");
    }
  }
  return n_bytes;
}


/**
 * DEFAULT PACKET DEFINITIONS
 */

ssize_t MatchmakingPacket::buildPacket(unsigned char* buf) {
  // Calculate buffer size
  // (
  //   sizeof(packet_type) + 
  //   sizeof(session_id) +
  //   sizeof(lobby_id) +
  //   sizeof(contents)
  // );

  // Keep track of where in the buffer we're currently copying
  size_t cur_index = 0;
  
  // Copy the type into the buffer
  memcpy(buf + cur_index, &packet_type, sizeof(packet_type));
  cur_index += sizeof(packet_type);

  // Copy the session id into the buffer
  packi64(buf+cur_index, session_id);
  cur_index += sizeof(session_id);

  // Copy the lobby id into the buffer
  packi64(buf+cur_index, lobby_id);
  cur_index += sizeof(lobby_id);

  // Copy the contents into the buffer
  contents[contents_size-1] = '\0'; // Guarantee safe cstr
  memcpy(buf + cur_index, contents, contents_size);

  if (ENABLE_CLIENTPACKET_INSPECTION){
    std::cout << getStringFromBuffer(buf, pkt_size);
  }

  return pkt_size;
}

std::string MatchmakingPacket::getStringFromBuffer(unsigned char* buf, ssize_t n_bytes) {
  std::stringstream ss;
  ss << " [Contents] packed type: " << (int)buf[0] << std::endl;
  ss << " [Contents] packed session: " << unpacku64(buf+sizeof(packet_type)) << std::endl;
  ss << " [Contents] packed lobby: " << unpacku64(buf+sizeof(packet_type)+sizeof(session_id)) << std::endl;
  char tmp_buf[BUFFER_SIZE];

  memcpy(
    tmp_buf, 
    buf+sizeof(packet_type)+sizeof(session_id)+sizeof(lobby_id), 
    contents_size
  );

  tmp_buf[BUFFER_SIZE-1] = '\0';
  ss << " [Contents] packed contents: " << tmp_buf << std::endl;
  return ss.str();
}

std::string MatchmakingPacket::getStringFromSelf() {
  std::stringstream ss;
  ss << " [Contents] type: " << (int)packet_type << std::endl;
  ss << " [Contents] session: " << session_id << std::endl;
  ss << " [Contents] lobby: " << lobby_id << std::endl;
  ss << " [Contents] contents: " << contents << std::endl;
  return ss.str();
}


/**
 * CLIENT PACKET DEFINITIONS
 */

ClientPacket::ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val) {
  pkt_size = CLIENT_PACKET_N_BYTES;
  contents_size = CLIENT_CONTENTS_SIZE;
  // Set first header value (no need to modify)
  packet_type = type;
  session_id = sid;
  lobby_id = lid;

  // Set contents (string copy and guarantee null terminator)
  strncpy(contents, val.c_str(), contents_size);
  contents[MAX_USERNAME_SIZE-1] = '\0';

  // Log it
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Log] ClientPacket initialized with " 
    << (int)type << " as type and "
    << contents << " as contents" << std::endl;
  }
}

ClientPacket::ClientPacket(unsigned char* buf, ssize_t n_bytes) {
  pkt_size = CLIENT_PACKET_N_BYTES;
  contents_size = CLIENT_CONTENTS_SIZE;

  // Copy to a local buffer to make sure all is well
  unsigned char tmp_buf[n_bytes];
  memcpy(tmp_buf, buf, n_bytes);

  packet_type = (uint8_t)tmp_buf[0];
  session_id = unpacku64(tmp_buf+sizeof(packet_type));
  lobby_id = unpacku64(tmp_buf+sizeof(packet_type)+sizeof(session_id));

  // Set contents
  memcpy(
    contents, 
    tmp_buf+sizeof(packet_type)+sizeof(session_id)+sizeof(lobby_id), 
    contents_size
  );
  // Guarantee null term
  contents[MAX_USERNAME_SIZE-1] = '\0';
}

/**
 * NET UTILS - courtesy of l'beej
 */

void packi64(unsigned char *buf, uint64_t i)
{
    *buf++ = i>>56; *buf++ = i>>48;
    *buf++ = i>>40; *buf++ = i>>32;
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}

/**
 * @brief unpack a 64-bit unsigned from a char buffer (like ntohl())
 */
uint64_t unpacku64(unsigned char *buf)
{
    return ((uint64_t)buf[0]<<56) |
           ((uint64_t)buf[1]<<48) |
           ((uint64_t)buf[2]<<40) |
           ((uint64_t)buf[3]<<32) |
           ((uint64_t)buf[4]<<24) |
           ((uint64_t)buf[5]<<16) |
           ((uint64_t)buf[6]<<8)  |
           buf[7];
}
