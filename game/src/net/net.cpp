/**
 * Definitions for local network functions
 * Built off of starter code from:  https://medium.com/@naseefcse/ip-tcp-programming-for-beginners-using-c-5bafb3788001
 * and: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-client-code
 */

#include "net.hpp"

#ifdef _WIN32
int testNetClient() {
  std::cout << "No windows netcode right now." << std::endl;
  return 0;
}
#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int NetEngine::testNetClient() {
  std::cout << "[Log] Running linux netcode" << std::endl;
  Timer timer;
  timer.start();
  int result, attempt_cnt, max_attempts = 3;
  ServerPacket in_pkt;

  // 1. Bind server connection to the returned socket
  result = serverConnect();
  if (server_sock < 0 || result < 0) {
    perror("[Error] Server connect request failed");
    serverDisconnect();
    return -1;
  }
  ClientPacket out_pkt(0, 69, 420, username);
  sendClientPacket(out_pkt);

  // 2. Send your username to initialize server connection
  // result = -1;
  // result = sendUserName();
  // if (result == 1) {
  //   serverDisconnect();
  //   return 1; // Retry username
  // }

  // Close the server connection
  serverDisconnect();

  // Connect to peer

  // Test peer connection

  // Close peer connection

  std::cout << "Network test finished in " << std::fixed << std::setprecision(17)
  << timer.duration() << "s\n";

  return 0;
}

NetEngine::NetEngine() {
  server_sock = -1;
  peer_sock = -1;
}

NetEngine::~NetEngine() {
  serverDisconnect();
  peerDisconnect();
}

int NetEngine::serverConnect() {
  struct sockaddr_in serv_addr;
  // Creating socket file descriptor
  if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    std::cerr << "[Error] Socket creation error" << std::endl;
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(SERVER_PORT);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, SERVER_ADDR, &serv_addr.sin_addr) <= 0) {
    std::cerr << "[Error] Invalid address/ Address not supported" << std::endl;
    return -1;
  }

  // Connect to the server
  if (connect(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      std::cerr << "[Error] Connection Failed" << std::endl;
    return -1;
  } else {
    printf("[Log] Connected to server\n");
  }
  return 0;
}

ssize_t NetEngine::sendClientPacket(ClientPacket out_pkt) {
  unsigned char buf[BUFFER_SIZE];

  // Populate buffer with packet contents (prepped for netsend)
  ssize_t pkt_size = out_pkt.buildPacket(buf);

  // Ensure full packet is sent
  ssize_t bytes_sent = 0, total_bytes_sent = 0;
  while (total_bytes_sent < pkt_size) {
    bytes_sent = send(server_sock, buf, pkt_size-total_bytes_sent, 0);
    total_bytes_sent += bytes_sent;
    if (bytes_sent < 0) {
      if (ENABLE_NETCODE_ERROR) {
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

ServerPacket NetEngine::recvServerPacket() {
  return ServerPacket();
}

void NetEngine::serverDisconnect() {
  if (server_sock > 0) {
    close(server_sock);
    server_sock = -1;
    if(ENABLE_NETCODE_LOG) {
      std::cout << "[Log] Disconnected from server.\n";
    }
  }
}

void NetEngine::peerDisconnect() {
  if (peer_sock > 0) {
    close(peer_sock);
    peer_sock = -1;
  }
}

#endif

ClientPacket::ClientPacket(uint8_t type, uint64_t sid, uint64_t lid, std::string val) {
  // Set first header value (no need to modify)
  packet_type = type;
  session_id = sid;
  lobby_id = lid;

  // Set contents (string copy and guarantee null terminator)
  strncpy(contents, val.c_str(), MAX_USERNAME_SIZE-1);
  contents[MAX_USERNAME_SIZE-1] = '\0';

  // Log it
  if (ENABLE_NETCODE_DEBUG) {
    std::cout << "[Log] ClientPacket initialized with " 
    << (int)type << " as type and "
    << contents << " as contents" << std::endl;
  }
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

  if (ENABLE_CLIENTPACKET_PRESEND_INSPECTION){
    std::cout << "[Debug] Output buffer size: " << pkt_size << std::endl;
    std::cout << "[Debug] type size: " << sizeof(packet_type) << std::endl;
    std::cout << "[Debug] session size: " << sizeof(session_id) << std::endl;
    std::cout << "[Debug] lobby size: " << sizeof(lobby_id) << std::endl;
    std::cout << "[Debug] contents size: " << sizeof(contents) << std::endl;

    std::cout << "[Debug] type: " << (int)packet_type << std::endl;
    std::cout << "[Debug] session: " << session_id << std::endl;
    std::cout << "[Debug] lobby: " << lobby_id << std::endl;
    std::cout << "[Debug] contents: " << contents << std::endl;
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

  if (ENABLE_CLIENTPACKET_PRESEND_INSPECTION){
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

void NetEngine::getLocalUserName() {
  std::string usr_name;
  std::cout << "Enter your username: " << std::endl;
  std::getline(std::cin, usr_name);
  // If bad input, repeat request until good
  while(usr_name.size() == 0 
  || usr_name.size() > MAX_USERNAME_SIZE-1
  || usr_name == "LIST"
  || usr_name == "CREATE") {
    std::cout << "Error] Invalid username: " << usr_name << std::endl;
    std::cout << "Error] Username must be 1-24 characters (" 
    << usr_name.size() << " is invalid.)" << std::endl;
    usr_name = "";
    std::getline(std::cin, usr_name);
  }
  // std::cout << "Username \"" << usr_name << "\" Received!\n";
  // Set the username in NetEngine
  setLocalUserName(usr_name);
}

void NetEngine::setLocalUserName(std::string user_name) {
  if (user_name.size() < MAX_USERNAME_SIZE) {
    username = user_name;
  } else {
    std::stringstream ss;
    ss << "[Error] ";
    ss << "Net engine failed to set username (" << user_name << ") ";
    ss << "too long (" << user_name.size() << " > " << MAX_USERNAME_SIZE-1 << ")";
    ss << std::endl;
    perror(ss.str().c_str());
  }
}

/**
 * NET UTILS
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