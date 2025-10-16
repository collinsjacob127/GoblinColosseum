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

// constexpr int SERVER_PORT = 0;
constexpr char* SERVER_PORT = "53243";
constexpr int BUFFER_SIZE = 1024;
constexpr int MAX_PENDING = 10;

struct PlayerEntry {
  uint32_t id = 0;            // Unique ID for this peer
  int socket_descriptor = 0;
  struct sockaddr_in address; // IP addr & port num

  std::string getReprString() {
    std::stringstream ss;
    char ip_buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address.sin_addr), ip_buffer, sizeof(ip_buffer));
    ss << "ID: " << id;
    ss << " Sock Desc: " << socket_descriptor;
    ss << " IP: " << ip_buffer << ":" << ntohs(address.sin_port);
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
int bind_and_listen(const char *service);


int main() {

  int opt = 1;

  // Tracking active sockets
  fd_set all_sockets, call_set;
  FD_ZERO(&all_sockets);

  // Socket where server accept() new connections thru
  int listen_socket = bind_and_listen(SERVER_PORT);
  FD_SET(listen_socket, &all_sockets);

  std::map<int, PlayerEntry> registry;
  std::cout << "Initial registry size: " << registry.size() << std::endl;

  // // Creating socket file descriptor
  // if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == 0) {
  //   perror("socket failed");
  //   exit(EXIT_FAILURE);
  // }

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

  // bool continue_server = true;
  // while (continue_server) {

  // Start listening for incoming connections
  if (listen(listen_socket, 3) < 0) {
    perror("listen");
    close(listen_socket);
    exit(EXIT_FAILURE);
  }


  // Accept incoming connection
  int new_socket;
  struct sockaddr_in new_address;
  socklen_t addr_len = sizeof(new_address);
  if ((new_socket = accept(listen_socket, (struct sockaddr*)&new_address, &addr_len)) < 0) {
    perror("accept");
    close(listen_socket);
    exit(EXIT_FAILURE);
  } else {
    // tmp_player_ent.id = 0;
    // tmp_player_ent.socket_descriptor = new_socket;
    // tmp_player_ent.address = address;
    // std::cout << tmp_player_ent.getReprString() << std::endl;
  }

  // Read and echo the received message
  char buffer[BUFFER_SIZE] = {0};
  ssize_t valread = read(new_socket, buffer, BUFFER_SIZE);
  std::cout << "Received: " << buffer << std::endl;
  send(new_socket, buffer, valread, 0);
  std::cout << "Echo message sent" << std::endl;

  // }

  // Close the socket
  close(new_socket);
  close(listen_socket);

  return 0;
}

int bind_and_listen(const char *service) {
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