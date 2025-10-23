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
constexpr size_t MAX_USERNAME_LEN = 25;
constexpr size_t MAX_PACKET_SIZE = 26;
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

struct PlayerEntry {
  uint32_t id = 0;            // Unique ID for this peer
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
  std::map<std::string, PlayerEntry> lobbies;
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
        int len;

        // Populate buffer with packet contents
        len = recv(s, buf, sizeof(buf), 0);

        // Verify valid packet size
        if (len > (int) MAX_PACKET_SIZE) {
          std::stringstream ss;
          ss << "[Error] ";
          ss << "Received invalid packet from: ";
          ss << registry.at(s).getReprString();
          perror(ss.str().c_str());
          disconnectPlayer(&registry, s, &all_sockets);
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
          char contents[MAX_USERNAME_LEN];
          strncpy(contents, buf+1, MAX_USERNAME_LEN);
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
              // CLIENT REQUESTING JOIN OR CREATE
              std::cout << "[Log] Received join/create notification: " << str_contents << std::endl;
              if (cur_client->user_name == "") {
                // Gotta send your own name first, bub
                continue;
              }

              if (str_contents == "JOIN") {
                // If joining -> send them list of current lobbies
                // Ensure valid state
                if (cur_client->open_lobby) {
                  // This mf breaking the rules
                  if (ENABLE_SERVER_ERROR) {
                    std::cerr << "[Error] Client JOIN with open_lobby flag. Closing them." << std::endl;
                  }
                  disconnectPlayer(&registry, s, &all_sockets);
                  continue;
                }

                // Gather current list of players with open lobbies
                std::vector<std::string> open_lobbies = getLobbyList(&registry);
                std::stringstream ss;
                ss << open_lobbies.size();
                std::string n_lobby_str = ss.str();
                // Send the lobby count first
                int n_bytes = send(s, n_lobby_str.c_str(), sizeof(n_lobby_str), 0);
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] Sent lobby count of " << n_lobby_str
                  << " (" << sizeof(n_lobby_str) << "b) to "
                  << cur_client->user_name << std::endl;
                }
                if (n_bytes < 0) {
                  perror("[Error] Failed to send client lobby count");
                  disconnectPlayer(&registry, s, &all_sockets);
                  break;
                }

                // Iterate through the list of lobbies and send them all
                for (size_t i = 0; i < open_lobbies.size(); i++) {
                  if (ENABLE_SERVER_DEBUG) {
                    std::cout << "[Debug] Sending lobby " << open_lobbies.at(i) << std::endl;
                  }
                  std::string lobby_uname = open_lobbies.at(i);
                  n_bytes = send(s, lobby_uname.c_str(), sizeof(lobby_uname), 0);
                  if (n_bytes < 0) {
                    perror("[Error] Error occured while sending lobby list");
                    disconnectPlayer(&registry, s, &all_sockets);
                    break;
                  }
                }
              } else if (str_contents == "CREATE") {
                // If creating -> make them a lobby
                cur_client->open_lobby = true;
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] " << cur_client->getReprString() 
                  << " marked with open_lobby\n";
                }
              } else {
                if (ENABLE_SERVER_ERROR) {
                  std::cerr << "[Error] Invalid JOIN/CREATE message received" << std::endl;
                }
              }
              break;
            }
            case 2: 
            {
              // CLIENT REQUESTING TO JOIN PEER'S LOBBY
              if (cur_client->match_made) {
                // What, how
                if(ENABLE_SERVER_ERROR) {
                  std::cerr << "[Error] Client trying to request multiple matches\n";
                }
                // continue;
                break;
              }

              std::cout << "[Log] Received lobby join request: " << str_contents << std::endl;
              PlayerEntry peer = getEntryFromUserName(&registry, contents);

              if (cur_client->user_name == "") {
                // Gotta send your own name first, bub
                // continue;
                break;
              }
              if (peer.match_made) {
                if(ENABLE_SERVER_ERROR) {
                  std::cerr << "[Error] Client trying to request match with busy peer\n";
                }
                // Match already made, yikes
                disconnectPlayer(&registry, s, &all_sockets);
                // continue;
                break;
              }

              // All good in the hood, send their data to eachother
              // Send peer addr to client
              std::stringstream ss;
              ss << peer.ipv4_str << ":" << peer.port_num;
              std::string peer_addr = ss.str();
              int n_bytes = send(s, peer_addr.c_str(), peer_addr.size(), 0);
              if (n_bytes < 0) {
                if (ENABLE_SERVER_ERROR) {
                  perror("[Error] Failed to send peer addr to client");
                }
              } else {
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] Sent peer addr to client: " << peer_addr << std::endl;
                }
              }

              // Send client addr to peer
              std::stringstream ss2;
              ss2 << cur_client->ipv4_str << ":" << cur_client->port_num;
              std::string client_addr = ss2.str();
              n_bytes = send(s, client_addr.c_str(), client_addr.size(), 0);
              if (n_bytes < 0) {
                if (ENABLE_SERVER_ERROR) {
                  perror("[Error] Failed to send client addr to peer");
                }
              } else {
                if (ENABLE_SERVER_LOG) {
                  std::cout << "[Log] Sent client addr to peer\n";
                }
              }

              // Remove their entries
              int s2 = peer.socket_descriptor;
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