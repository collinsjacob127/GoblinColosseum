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
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 10;

struct PlayerEntry {
  uint32_t id = 0;            // Unique ID for this peer
  int socket_descriptor = 0;
  struct sockaddr_in address; // IP addr & port num
  std::string ipv4_str = "";
  uint16_t port_num;

  PlayerEntry(int sock_desc, sockaddr_in addr) {
    socket_descriptor = sock_desc;
    address = addr;

    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ipv4_str = std::string(ip_buffer);

    port_num = ntohs(address.sin_port);
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

int main() {
  Timer server_timer;
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
    exit(EXIT_FAILURE);
  }
  FD_SET(listen_socket, &all_sockets);

  // always equal to the max fd from which the program can accept() new conns
  int max_socket = listen_socket;

  // Maps fd -> Player Entry
  std::map<int, PlayerEntry> registry;
  std::cout << "Initial registry size: " << registry.size() << std::endl;

  // // Creating socket file descriptor
  // if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
  //   perror("socket failed");
  //   exit(EXIT_FAILURE);
  // }
  // int opt = 1;
  // // Forcefully attaching socket to the port
  // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
  //   perror("setsockopt");
  //   exit(EXIT_FAILURE);
  // }
  // // Set server properties
  // server_addr.sin_family = AF_INET;
  // server_addr.sin_addr.s_addr = INADDR_ANY;
  // server_addr.sin_port = htons(SERVER_PORT);
  // // Bind the socket to the network address and port
  // if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
  //   perror("bind failed");
  //   exit(EXIT_FAILURE);
  // }
  // // Get server's socket info
  // if (getsockname(server_fd, (struct sockaddr *)&address, &addr_len) < 0) {
  //   perror("getsockname failed");
  //   close(server_fd);
  //   exit(EXIT_FAILURE);
  // }
  // PlayerEntry tmp_player_ent;
  // tmp_player_ent.id = 999;
  // tmp_player_ent.socket_descriptor = server_fd; 
  // tmp_player_ent.address = address;
  // std::cout << "Server Info: " << tmp_player_ent.getReprString() << std::endl;

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
        if (len < 0) {
          /* Quit on error */
          perror("Error occured in recv() call, closing and exiting...");
          closeAllInSet(&all_sockets, 3, max_socket);
          exit(EXIT_FAILURE);
        } else if (len == 0) {
          /* Recieved empty */
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

          std::string out_msg = "The server says hello!";
          send(s, out_msg.c_str(), out_msg.size(), 0);
          std::cout << "Sent: " << out_msg << std::endl;
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