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

#include <iostream>
#include <string>
#include <string.h>
#include <sstream>
#include <map>
#include <csignal> // Handle SIGINT

#include <memory>
#include <cstring>

#include <netdb.h>
// #include <sys/poll.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.hpp"

#define ENABLE_SERVER_LOG true
#define ENABLE_SERVER_DEBUG true
#define ENABLE_SERVER_ERROR true

// constexpr int SERVER_PORT = 0;
constexpr char* SERVER_PORT = (char*)"53243";
constexpr ssize_t MAX_USERNAME_SIZE = 25;
constexpr ssize_t MAX_PACKET_SIZE = 26;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 64;

/**
 * GLOBALS for server; Necessary for cleaning up from SIGINT
 */
// all sockets -> all active
// call_set -> oft overwritten, only used for select()
fd_set all_sockets, call_set;
int listen_socket;
int min_fd = 3, max_socket;

struct ClientPacket {
  uint8_t packet_type = 0;

  // Random number assigned by server. Used to self-identify when making requests.
  uint64_t session_id = 0;
  // Lobby number assigned by server.
  uint64_t lobby_id = 0;

  /**
   * Contents types (c-string):
   * @note 0 -> Own username
   * @note 1 -> "CREATE"
   * @note 2 -> "LIST"
   * @note 3 -> Peer username (?)
  */
  char contents[MAX_USERNAME_SIZE];

  /**
   * @brief Build a client packet given the type
   * and contents
   * @param type Header to inform server what type of message is being sent
   * @param val Contents of the message being sent
   * @note Possible values for type:
   * @note 0 -> Sending own username (Entering server). Expects server response "GOOD"
   * @note 1 -> (CREATE) Requesting to create a lobby. Expects server sesponse "GOOD"
   * @note 2 -> (LIST) Requesting list of available peers. 
   * @note 3 -> Requesting ip addr of peer. 
   */
  ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val);

  /**
   * @brief Function to move ClientPacket into a provided
   * buffer and prepare the contents for network send
   */
  ssize_t buildPacket(unsigned char* buf);

  std::string getStringFromBuffer(unsigned char* buf, ssize_t n_bytes);
};

struct PlayerEntry {
  uint64_t id = 0;            // Unique ID for this peer
  std::string user_name = "";
  bool match_made = false;
  bool open_lobby = false;
  int socket_descriptor = 0;
  struct sockaddr_in address; // IP addr & port num
  std::string ipv4_str = "";
  uint16_t port_num;
  Timer p_timer;
  Timer lobby_update_time; 

  PlayerEntry() { p_timer.start(); }
  PlayerEntry(int sock_desc, sockaddr_in addr) {
    // Set address info for new client
    socket_descriptor = sock_desc;
    address = addr;

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ipv4_str = std::string(ip_buffer);

    port_num = ntohs(address.sin_port);

    // Start their timer
    p_timer.start();
  }

  std::string getReprString() {
    std::stringstream ss;

    // Set ipv4 str if not yet set
    if (ipv4_str == "") {
      char ip_buffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
      ipv4_str = std::string(ip_buffer);
    }

    ss << "ID: " << id;
    ss << " Sock Desc: " << socket_descriptor;
    ss << " IP: " << ipv4_str << ":" << ntohs(address.sin_port);
    return ss.str();
  }
};

/**
 * @brief Create, bind and passive open a socket on a local interface for the provided
 * service. Argument matches the second argument to getaddrinfo(3).
 *
 * @return Passively opened socket or -1 on error. Caller is responsible for
 * calling accept and closing the socket.
 */
int bindAndListen(const char *service);

/**
 * @brief Close every socket in a file descriptor set.
 */
void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd);

/**
 * @brief Given a username, return the corresponding registry entry.
 */
PlayerEntry getEntryFromUserName(std::map<int, PlayerEntry> *registry, std::string uname);

/**
 * @brief Player uname requests the list of open lobbies.
 * If they are marked as open_lobby, get rid of that 'cause they shouldn't be.
 * Don't return themselves.
 */
std::vector<std::string> getLobbyList(std::map<int, PlayerEntry> *registry);

/**
 * @brief Function to disconnect a client from the registry etc.
 */
void disconnectPlayer(std::map<int, PlayerEntry> *registry, int fd, fd_set *sock_set);

/**
 * @brief Function to send a packet to given client with contents "GOOD" or "BAD".
 * @return returns bytes sent or -1 on error.
 */
ssize_t sendConfirmationResponse(int fd, bool is_good);

void handleSigint(int signal_num);

int main() {
  std::signal(SIGINT, handleSigint);

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
