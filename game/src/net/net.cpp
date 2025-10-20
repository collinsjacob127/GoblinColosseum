/**
 * Definitions for local network functions
 * Built off of starter code from:  https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 * and: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
 */

#include "net.hpp"

constexpr int SERVER_PORT = 53243;
constexpr int BUFFER_SIZE = 1024;

//DELETE THIS
#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")


int testNetClient() {
  std::cout << "Running windows netcode" << std::endl;
  Timer timer;
  timer.start();

  // Do boilerplate winsock stuff 
  WSADATA wsaData;
  int iResult;
  WSAStartup(MAKEWORD(2, 2), &wsaData);
  char buffer[BUFFER_SIZE] = {0};

  SOCKET connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr); // Connect to localhost
  serverAddr.sin_port = htons(SERVER_PORT); // Example port

  connect(connectSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
  // ... send and receive data with connectSocket ...
  if (connectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
  }

  // Send test packet thrice
  for (int i = 0; i < 3; ++i) {
    // Send one per 2 second
    if (timer.duration() < (double)i * 2) {
      --i;
      continue;
    }

    std::stringstream ss;
    ss << "Hello #" << i << " from Windows client!" << std::endl;
    std::string hello = ss.str();

    iResult = send( connectSocket, hello.c_str(), hello.size(), 0 );
    if (iResult == SOCKET_ERROR) {
      printf("send failed with error: %d\n", WSAGetLastError());
      closesocket(connectSocket);
      WSACleanup();
      return 1;
    }
    printf("Bytes Sent: %ld\n", iResult);
  }


  // shutdown the connection since no more data will be sent
  iResult = shutdown(connectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(connectSocket);
    WSACleanup();
    return 1;
  }

// Receive until the peer closes the connection
  do {

    iResult = recv(connectSocket, buffer, BUFFER_SIZE, 0);
    if ( iResult > 0 ) {
      printf("Bytes received: %d\n", iResult);
      std::cout << "Recieved: " << buffer << std::endl;
    } else if ( iResult == 0 )
      printf("Connection closed\n");
    else
      printf("recv failed with error: %d\n", WSAGetLastError());

  } while( iResult > 0 );

  closesocket(connectSocket);
  WSACleanup();
  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";
  return 0;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int testNetClient() {
  std::cout << "Running linux netcode" << std::endl;
  Timer timer;
  timer.start();

  /**
   * Connecting to server
   */
  int sock = 0;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE] = {0};

  // Creating socket file descriptor
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "Socket creation error" << std::endl;
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
    std::cerr << "Invalid address/ Address not supported" << std::endl;
    return -1;
  }

  // Connect to the server
  if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    std::cerr << "Connection Failed" << std::endl;
    return -1;
  } else {
    printf("Connected to server\n");
  }

  printf("Provide your username:\n");
  std::string usr_name = getUserName();
  send(sock, usr_name.c_str(), usr_name.size(), 0);
  std::cout << "Username sent: " << usr_name << std::endl;

  ssize_t valread = read(sock, buffer, BUFFER_SIZE);
  std::cout << "Received: " << buffer << std::endl;

  // Close the socket
  close(sock);

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}

#endif

std::string getUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::cin >> usr_name;
  while(usr_name.size() == 0 || usr_name.size() > MAX_USERNAME_LEN-1) {
    std::cout << "Error] Invalid username: " << usr_name << std::endl;
    std::cout << "Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid." << std::endl;
    usr_name = "";
    std::cin >> usr_name;
  }
  return usr_name;
}