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

#include <memory>
#include <cstring>

#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "util.hpp"

// constexpr int SERVER_PORT = 0;
constexpr char* SERVER_PORT = (char*)"53243";
constexpr size_t MAX_USERNAME_LEN = 25;
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 1000;

struct PlayerEntry {
  uint32_t id = 0;            // Unique ID for this peer
  std::string user_name = "";
  bool match_made = false;
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

void closeAllInSet(fd_set *socket_list, int min_fd, int max_fd);

PlayerEntry getEntryFromUserName(std::map<int, PlayerEntry> *registry, std::string uname);

int main() {
  // Timer to track how long the server has been up
  Timer server_timer;
  // Timer to track the last time the registry was updated
  Timer reg_timer;
  server_timer.start();

  // all sockets -> all active
  // call_set -> oft overwritten, only used for select()
  fd_set all_sockets, call_set;
  FD_ZERO(&all_sockets);

  // Socket where server accept() new connections thru
  int listen_socket = bindAndListen(SERVER_PORT);
  int opt = 1;
  if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
    perror("setsockopt");
    close(listen_socket);
    exit(EXIT_FAILURE);
  }
  FD_SET(listen_socket, &all_sockets);

  // always equal to the max fd from which the program can accept() new conns
  int max_socket = listen_socket;

  // Maps fd -> Player Entry
  std::map<int, PlayerEntry> registry;
  std::cout << "Initial registry size: " << registry.size() << std::endl;
  printf("Server has started!\n");

  bool continue_server = true;
  while (continue_server) {
    // printf("%lf\r", server_timer.duration());
    // std::cout << std::flush;
    if (server_timer.duration() > 60.0) { continue_server = false; }

    call_set = all_sockets;
    // Select can only handle 1024 fds; update to poll()
    timeval timeout_dur;
    timeout_dur.tv_sec = 0;
    timeout_dur.tv_usec = 100'000;
    int num_s = select(max_socket + 1, &call_set, NULL, NULL, &timeout_dur);
    if (num_s < 0) {
      perror("ERROR from select() call");
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
          perror("Invalid accept attempted, closing server and exiting...");
          closeAllInSet(&all_sockets, 3, max_socket);
          exit(EXIT_FAILURE);
        }

        // Populate PlayerEntry
        PlayerEntry new_player(new_socket, new_address);
        new_player.p_timer.start();
        // Insert to registry map
        registry.insert_or_assign(new_socket, new_player);
        printf("A client has connected via socket %d from %s:%u.\n", 
          new_socket, 
          new_player.ipv4_str.c_str(),
          new_player.port_num
        );

        // Add this socket to file descriptor set
        FD_SET(new_socket, &all_sockets);

        // Update max socket
        max_socket = std::max(new_socket, max_socket);

        // Update the reg timer
        reg_timer.start();
      } else {
        // Handling EXISTING connection

        // Remove closed connections from our client list
        if (!FD_ISSET(s, &call_set)) {
          printf("this shouldn't happen\n");
          continue;
        }

        char buf[BUFFER_SIZE] = {0};
        int len;

        // Populate buffer with packet contents
        len = recv(s, buf, sizeof(buf), 0);

        // Verify valid packet size
        if (len > MAX_USERNAME_LEN) {
          close(s);
          FD_CLR(s, &all_sockets);
          std::stringstream ss;
          ss << "Recieved invalid username from: ";
          ss << registry.at(s).getReprString();
          perror(ss.str().c_str());
        }

        if (len < 0) {
          /* Quit on error */

          perror("Error occured in recv() call, closing and exiting...");
          closeAllInSet(&all_sockets, 3, max_socket);
          exit(EXIT_FAILURE);

        } else if (len == 0) {
          /* Recieved empty */

          // Only close if a match has been made for this user
          if (!registry.at(s).match_made) { continue; }

          // Match has been made, user connection finished
          close(s);
          FD_CLR(s, &all_sockets);
          printf("User %d @ %s:%u disconnected.\n", 
            registry.at(s).id,
            registry.at(s).ipv4_str.c_str(),
            registry.at(s).port_num
          );
          registry.erase(s);

        } else {
          /* Recieved non-empty */

          std::cout << "Received: " << buf << std::endl;
          PlayerEntry *cur_client = &registry.at(s);

          // Check if user has already provided their name
          if (cur_client->user_name == "") {
            // No name -> Recieve username from client
            cur_client->user_name = buf;
            // Now, send client the list of current connections

          } else {
            // Yes name -> Recieve username of peer client wishes to connect with
            PlayerEntry tmp = getEntryFromUserName(&registry, buf);

            // Check that user not requesting self
            if (tmp.user_name == cur_client->user_name) {
              std::stringstream ss;
              ss << "Matchmaking error, user requested self:\n";
              ss << cur_client->getReprString().c_str(); 
              perror(ss.str().c_str());
              continue;
            }

            // Check that user not requesting invalid 
            if (tmp.user_name == "") {
              std::stringstream ss;
              ss << "Matchmaking error, non-existant peer requested by user:\n";
              ss << cur_client->getReprString().c_str(); 
              perror(ss.str().c_str());
              continue;
            }

            // All good, send the peer's info
            std::stringstream ss;
            ss << tmp.ipv4_str << ":" << tmp.port_num;
            std::string out_msg = ss.str();

            send(s, out_msg.c_str(), out_msg.size(), 0);

            std::cout << "Sent: " << out_msg << "to client:" 
            << cur_client->getReprString() << std::endl;

            // Register that match has been made for both peers and they
            // can safely disconnect from server
            cur_client->match_made = true;
            registry.at(tmp.socket_descriptor).match_made = true;
          }

        }
      }
    }


  }

  // Close the server's socket
  closeAllInSet(&all_sockets, 3, max_socket);
  printf("Server closing...\n");
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
  for (size_t i = 0; i < registry->size(); ++i) {
    if (registry->at(i).user_name == uname) {
      tmp = registry->at(i);
    }
  }
  return tmp;
}
