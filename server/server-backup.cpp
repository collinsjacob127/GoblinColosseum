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
fd_set all_sockets, call_set;
int listen_socket;
int min_fd = 3, max_socket;

int main() {
  // Safely clean up the server if shit breaks.
  std::signal(SIGINT, handleSigint);
  std::signal(SIGABRT, handleSigint);
  std::signal(SIGTERM, handleSigint);

  // Timer to track how long the server has been up
  Timer server_timer;
  server_timer.start();

  FD_ZERO(&all_sockets);

  // Socket where server accept() new connections thru
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

  // Maps fd -> Player Entry
  std::map<int, PlayerEntry> registry;
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Initial registry size: " << registry.size() << std::endl;
  }
  if (ENABLE_SERVER_LOG) {
    printf("[Log] Server has started!\n");
  }

  bool continue_server = true;
  while (continue_server) {
    if (server_timer.duration() > 120.0) { continue_server = false; }

    call_set = all_sockets;
    // Select can only handle 1024 fds; update to poll()
    timeval timeout_dur;
    timeout_dur.tv_sec = 0;
    timeout_dur.tv_usec = 100'000;
    int num_s = select(max_socket + 1, &call_set, NULL, NULL, &timeout_dur);
    if (num_s < 0) {
      perror("[Error] from select() call");
      closeAllInSet(&all_sockets, 3, max_socket);
      exit(EXIT_FAILURE);
    }

    // Loop through all possible sockets (skipping std in/out/err)
    for (int s = 3; s <= max_socket; ++s) {
      // Skip unready sockets
      if (!FD_ISSET(s, &call_set))
        continue;

      // New connection available
      if (s == listen_socket) {
        // Handling NEW connection

        // Bind socket to local interface and passive open
        int new_socket;
        struct sockaddr_in new_address;
        socklen_t addr_len = sizeof(new_address);
        if ((new_socket = accept(s, (struct sockaddr*)&new_address, &addr_len)) < 0) {
          perror("[Error] Invalid accept attempted, closing server and exiting...");
          closeAllInSet(&all_sockets, 3, max_socket);
          exit(EXIT_FAILURE);
        }

        // Populate PlayerEntry
        PlayerEntry new_player(new_socket, new_address);
        new_player.p_timer.start();
        // Insert to registry map
        registry.insert_or_assign(new_socket, new_player);
        printf("[Log] A client has connected via socket %d from %s:%u.\n", 
          new_socket, 
          new_player.ipv4_str.c_str(),
          new_player.port_num
        );

        // Add this socket to file descriptor set
        FD_SET(new_socket, &all_sockets);

        // Update max socket
        max_socket = std::max(new_socket, max_socket);
      } else {
        // Handling EXISTING connection

        // Remove closed connections from our client list
        if (!FD_ISSET(s, &call_set)) {
          printf("[WTF] this shouldn't happen\n");
          continue;
        }

        char buf[BUFFER_SIZE] = {0};
        ssize_t len;

        // Populate buffer with packet contents
        len = recv(s, buf, sizeof(buf), 0);

        // Verify valid packet size
        if (len > MAX_PACKET_SIZE) {
          std::stringstream ss;
          ss << "[Error] ";
          ss << "Received invalid packet from: ";
          ss << registry.at(s).getReprString();
          perror(ss.str().c_str());
          disconnectPlayer(&registry, s, &all_sockets);
          continue;
        }

        if (len < 0) {
          /* Quit on error */
          std::stringstream ss;
          ss << "Error occured in recv() call, disconnecting player: ";
          ss << registry.at(s).user_name;
          perror(ss.str().c_str());
          disconnectPlayer(&registry, s, &all_sockets);
        } else if (len == 0) {
          /* Received empty */

          // Only close if a match has been made for this user
          if (!registry.at(s).match_made) { continue; }

          // Match has been made, user connection finished
          disconnectPlayer(&registry, s, &all_sockets);
        } else {
          /* Received non-empty */

          if (ENABLE_SERVER_DEBUG) {
            std::cout << "[Debug] Len of Received: " << len << std::endl;
          }
          PlayerEntry *cur_client = &registry.at(s);

          char pkt_type = buf[0];
          char contents[MAX_USERNAME_SIZE];
          strncpy(contents, buf+1, MAX_USERNAME_SIZE);
          std::string str_contents = contents;

          switch (pkt_type) {
            case 0: 
            {
              // CLIENT INITIALIZING WITH THEIR USERNAME
              std::cout << "[Log] Received username: " << str_contents << std::endl;
              // Check unique
              PlayerEntry tmp = getEntryFromUserName(&registry, str_contents);
              std::string out_msg;
              if (tmp.user_name == "") {
                out_msg = "GOOD";
                send(s, out_msg.c_str(), sizeof(out_msg), 0);
              } else {
                // Username already taken
                out_msg = "BAD";
                send(s, out_msg.c_str(), sizeof(out_msg), 0);
                // Begone
                disconnectPlayer(&registry, s, &all_sockets);
                // continue;
                break;
              }
              // Set username
              cur_client->user_name = str_contents;
              break;
            }
            case 1: 
            {
              // CLIENT REQUESTING TO CREATE LOBBY
              std::cout << "[Log] Received create notification: " << str_contents << std::endl;
              ssize_t n_bytes;
              if (cur_client->user_name == "") {
                // Gotta send your own name first, bub
                // Send failure packet
                if ((n_bytes = sendConfirmationResponse(s, false)) < 0) {
                  perror("[Error] Failed to send BAD response\n");
                }
                disconnectPlayer(&registry, s, &all_sockets);
                continue;
              }

              // If creating -> make them a lobby
              cur_client->open_lobby = true;
              if (ENABLE_SERVER_LOG) {
                std::cout << "[Log] User " << cur_client->user_name << " marked with open_lobby\n";
              }

              // Send success packet
              if ((n_bytes = sendConfirmationResponse(s, true)) < 0) {
                disconnectPlayer(&registry, s, &all_sockets);
              } else {
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] GOOD response successfully sent. \n";
                }
              }

              break;
            }
            case 2:
            {
              // Requesting list of lobbies

              break;
            }
            case 3: 
            {
              // Requesting IP info of peer
              std::cout << "[Log] Received peer addr request: " << str_contents << std::endl;
              PlayerEntry peer = getEntryFromUserName(&registry, contents);
              ssize_t n_bytes;

              if (cur_client->user_name == "" || peer.user_name == "") {
                // Gotta send your own name first, bub OR bad user request
                char tmp[6] = {0};
                if ((n_bytes = send(s, tmp, 6, 0)) < 0) {
                  perror("[Error] Failed to send addr request failure notif\n");
                }
                break;
              }

              if (peer.match_made) {
                if(ENABLE_SERVER_ERROR) {
                  std::cerr << "[Error] Client trying to request match with busy peer\n";
                }
                // Match already made, yikes
                sendConfirmationResponse(s, false);
                disconnectPlayer(&registry, s, &all_sockets);
                // continue;
                break;
              }

              // All good in the hood, send their data to eachother
              int s2 = peer.socket_descriptor;
              // Get addr and port of peer
              uint32_t peer_addr = htonl(peer.address.sin_addr.s_addr);
              uint16_t peer_port = htons(peer.port_num);
              unsigned char out_buf[6];
              // Copy into output buffer
              memcpy(out_buf, &peer_addr, 4);
              memcpy(out_buf+4, &peer_port, 2);
              // Send peer addr to client
              if ((n_bytes = send(s, out_buf, 6, 0)) < 0) {
                if (ENABLE_SERVER_ERROR) {
                  perror("[Error] Failed to send peer addr to client");
                }
              } else {
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] Sent peer addr to client: " << peer_addr << std::endl;
                }
              }

              // Send client addr to peer
              uint32_t cli_addr = htonl(cur_client->address.sin_addr.s_addr);
              uint16_t cli_port = htons(cur_client->port_num);
              // Copy into output buffer
              memcpy(out_buf, &cli_addr, 4);
              memcpy(out_buf+4, &cli_port, 2);
              if ((n_bytes = send(s2, out_buf, 6, 0)) < 0) {
                if (ENABLE_SERVER_ERROR) {
                  perror("[Error] Failed to send client addr to peer");
                }
              } else {
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] Sent client addr to peer\n";
                }
              }

              // Remove their entries
              disconnectPlayer(&registry, s, &all_sockets);
              disconnectPlayer(&registry, s2, &all_sockets);

              break;
            }
            default: 
            {
              // INVALID HEADER
              std::cerr << "Invalid packet type received ("
              << pkt_type << "). Aborting." << std::endl;
              closeAllInSet(&all_sockets, 3, max_socket);
              return -1;
            }
          }
        }
      }
    }


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
PlayerEntry getEntryFromUserName(std::map<int, PlayerEntry> *registry, std::string uname) {
  PlayerEntry tmp;
  std::map<int, PlayerEntry>::iterator it;
  // Iterate through registry
  for (it = registry->begin(); it != registry->end(); it++) {
    // Check if user has matching name
    if (it->second.user_name == uname) {
      tmp = it->second;
    }
  }
  return tmp;
}

std::vector<std::string> getLobbyList(std::map<int, PlayerEntry> *registry) {
  std::vector<std::string> lobby_list;
  std::map<int, PlayerEntry>::iterator it;
  // Iterate through registry
  for (it = registry->begin(); it != registry->end(); it++) {
    // Check if user has open lobby
    if (it->second.open_lobby) {
      lobby_list.push_back(it->second.user_name);
    }
  }
  return lobby_list;
}

void disconnectPlayer(std::map<int, PlayerEntry> *registry, int fd, fd_set *sock_set) {
  close(fd);
  FD_CLR(fd, sock_set);
  printf("[Log] User %d @ %s:%u disconnected.\n", 
    registry->at(fd).id,
    registry->at(fd).ipv4_str.c_str(),
    registry->at(fd).port_num
  );
  std::map<int, PlayerEntry>::iterator it = registry->find(fd);
  registry->erase(it);
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Debug] Server registry now has " << registry->size() << " entries\n";
  }
}

void handleSigint(int signal_num) {
  std::cerr << "\n[Error] Caught SIGINT - Disconnecting all clients and shutting down server.\n";
  closeAllInSet(&all_sockets, min_fd, max_socket);
  FD_ZERO(&all_sockets);
  FD_ZERO(&call_set);
  std::cout << "[Log] Server shutting down safely.\n";
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
 * CLIENTPACKET DEFINITIONS
 */

ClientPacket::ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val) {
  // Set first header value (no need to modify)
  packet_type = type;
  session_id = sid;
  lobby_id = lid;

  // Set contents (string copy and guarantee null terminator)
  strncpy(contents, val.c_str(), MAX_USERNAME_SIZE-1);
  contents[MAX_USERNAME_SIZE-1] = '\0';

  // Log it
  if (ENABLE_SERVER_DEBUG) {
    std::cout << "[Log] ClientPacket initialized with " 
    << (int)type << " as type and "
    << contents << " as contents" << std::endl;
  }
}

ClientPacket::ClientPacket(unsigned char* buf, ssize_t n_bytes) {
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
    sizeof(contents)
  );
  // Guarantee null term
  contents[MAX_USERNAME_SIZE-1] = '\0';
}

ssize_t ClientPacket::buildPacket(unsigned char* buf) {
  // Calculate buffer size
  size_t pkt_size = 
  (
    sizeof(packet_type) + 
    sizeof(session_id) +
    sizeof(lobby_id) +
    sizeof(contents)
  );

  if (ENABLE_CLIENTPACKET_INSPECTION){
    std::cout << "[Debug] Output buffer size: " << pkt_size << std::endl;
    std::cout << "[Debug] type size: " << sizeof(packet_type) << std::endl;
    std::cout << "[Debug] session size: " << sizeof(session_id) << std::endl;
    std::cout << "[Debug] lobby size: " << sizeof(lobby_id) << std::endl;
    std::cout << "[Debug] contents size: " << sizeof(contents) << std::endl;
    std::cout << getStringFromSelf();
  }

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
  contents[MAX_USERNAME_SIZE-1] = '\0'; // Guarantee safe cstr
  memcpy(buf + cur_index, contents, sizeof(contents));

  if (ENABLE_CLIENTPACKET_INSPECTION){
    std::cout << getStringFromBuffer(buf, pkt_size);
  }

  return pkt_size;
}

std::string ClientPacket::getStringFromBuffer(unsigned char* buf, ssize_t n_bytes) {
  std::stringstream ss;
  ss << "[Debug] packed type: " << (int)buf[0] << std::endl;
  ss << "[Debug] packed session: " << unpacku64(buf+sizeof(packet_type)) << std::endl;
  ss << "[Debug] packed lobby: " << unpacku64(buf+sizeof(packet_type)+sizeof(session_id)) << std::endl;
  char tmp_buf[BUFFER_SIZE];

  memcpy(
    tmp_buf, 
    buf+sizeof(packet_type)+sizeof(session_id)+sizeof(lobby_id), 
    sizeof(contents)
  );

  tmp_buf[BUFFER_SIZE-1] = '\0';
  ss << "[Debug] packed contents: " << tmp_buf << std::endl;
  return ss.str();
}

std::string ClientPacket::getStringFromSelf() {
  std::stringstream ss;
  ss << "[Debug] type: " << (int)packet_type << std::endl;
  ss << "[Debug] session: " << session_id << std::endl;
  ss << "[Debug] lobby: " << lobby_id << std::endl;
  ss << "[Debug] contents: " << contents << std::endl;
  return ss.str();
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
