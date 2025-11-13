
#include "packets.hpp"

/**
 * ADDR INFO DEFINITIONS
 */

clientAddrInfo::clientAddrInfo(std::string ipv4_str) {
  // Verify not too small
  if (ipv4_str.size() < 9) { return; }
  // Verify not too big
  if (ipv4_str.size() > CLIENT_CONTENTS_SIZE) { return; }

  size_t colon_pos;
  if ((colon_pos = ipv4_str.find(':')) == std::string::npos) {
    std::cout << "[Error] Error converting IP address: " << ipv4_str << std::endl; 
    return;
  }

  // Select substrings
  std::string addr_str = ipv4_str.substr(0, colon_pos);
  if (addr_str.length() == 0) { return; }
  std::string port_str = ipv4_str.substr(colon_pos+1);
  if (port_str.length() == 0) { return; }

  // Convert ipv4 addr
  std::string cur_str, remainder=addr_str;
  uint8_t ipv4_buf[4];
  // First 3 numbers
  for (size_t i = 0; i < 3; ++i) {
    // Get index of next colon
    if ((colon_pos = remainder.find_first_of('.')) == std::string::npos) { return; }
    // Current number (until next period)
    cur_str = remainder.substr(0, colon_pos);
    // Update remainder, removing cur number & period
    remainder = remainder.substr(colon_pos + 1);
    // Send current number to buffer
    ipv4_buf[3-i] = (uint8_t)atoi(cur_str.c_str());
  }
  ipv4_buf[0] = (uint8_t)atoi(remainder.c_str());
  memcpy(&addr, ipv4_buf, 4);

  // Convert port
  port = (uint16_t)atoi(port_str.c_str());

  rep_str = ipv4_str;
}

clientAddrInfo::clientAddrInfo(uint32_t in_addr, uint16_t in_port) {
  addr = unpacku32((unsigned char*)&in_addr);
  port = unpacku16((unsigned char*)&in_port);
  std::stringstream ss;
  ss << (int)(((unsigned char*)&addr)[3]);
  ss << ".";
  ss << (int)(((unsigned char*)&addr)[2]);
  ss << ".";
  ss << (int)(((unsigned char*)&addr)[1]);
  ss << ".";
  ss << (int)(((unsigned char*)&addr)[0]);
  ss << ":" << port;
  rep_str = ss.str();
}



/**
 * DEFAULT PACKET DEFINITIONS
 */

ssize_t MatchmakingPacket::buildPacket(unsigned char* buf) {
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
  contents[CLIENT_CONTENTS_SIZE-1] = '\0';

  // Log it
  if (ENABLE_CLIENTPACKET_INSPECTION) {
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
  contents[CLIENT_CONTENTS_SIZE-1] = '\0';
}

/**
 * SERVER PACKET DEFINITIONS
 */

ServerPacket::ServerPacket(uint8_t type, uint64_t sid, uint64_t lid, const char *val) {
  pkt_size = SERVER_PACKET_N_BYTES;
  contents_size = SERVER_CONTENTS_SIZE;
  // Set first header value (no need to modify)
  packet_type = type;
  session_id = sid;
  lobby_id = lid;

  // Set contents (string copy and guarantee null terminator)
  memcpy(
    contents, 
    val, 
    contents_size
  );
  // Guarantee null term
  contents[SERVER_CONTENTS_SIZE-1] = '\0';
  contents[SERVER_CONTENTS_SIZE-1] = '\0';

  // Log it
  if (ENABLE_CLIENTPACKET_INSPECTION) {
    std::cout << "[Log] ServerPacket initialized with " 
    << (int)type << " as type and "
    << contents << " as contents" << std::endl;
  }
}

ServerPacket::ServerPacket(unsigned char* buf, ssize_t n_bytes) {
  pkt_size = SERVER_PACKET_N_BYTES;
  contents_size = SERVER_CONTENTS_SIZE;

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
  contents[SERVER_CONTENTS_SIZE-1] = '\0';
}

ServerPacket::ServerPacket(uint8_t type, uint64_t n_sent, uint64_t n_total, std::vector<TYPE_LOBBY_INFO> lobby_list) {
  pkt_size = SERVER_PACKET_N_BYTES;
  contents_size = SERVER_CONTENTS_SIZE;
  // Set first header value (no need to modify)
  packet_type = type;
  session_id = n_sent;
  lobby_id = n_total;

  // Verify valid fill request
  if (lobby_list.size() > MAX_N_REQUESTED_LOBBIES) {
    std::cout << "[PACKET ERROR] Invalid lobby fill request\n";
  }

  unsigned char id_buf[sizeof(uint64_t)];

  // Loop through list of lobbies
  for (size_t i = 0; i < lobby_list.size(); ++i) {
    // Get lobby name
    const char *cur_lobby_name = lobby_list.at(i).first.c_str();
    char* contents_idx_ptr = contents + (CLIENT_CONTENTS_SIZE * 2 * i);

    // Copy lobby name into every other buffer segment
    strncpy(contents_idx_ptr, cur_lobby_name, (size_t)CLIENT_CONTENTS_SIZE);
    
    // Pack ID buffer in the temp buffer
    packi64(id_buf, lobby_list.at(i).second);

    // Copy Packed ID to contents in the between segments
    memcpy(contents_idx_ptr + CLIENT_CONTENTS_SIZE, id_buf, sizeof(uint64_t));

    if (ENABLE_CLIENTPACKET_INSPECTION) {
      std::cout << "[Debug] Adding lobby #" << i << ": " << cur_lobby_name << std::endl;
    }
  }

}

ServerPacket::ServerPacket(uint8_t type, uint64_t sid, uint64_t lid, clientAddrInfo peer_addr_pub, clientAddrInfo peer_addr_priv) {
  // Standard initialization
  pkt_size = SERVER_PACKET_N_BYTES;
  contents_size = SERVER_CONTENTS_SIZE;
  // Paramaterized initialization
  packet_type = type;
  session_id = sid;
  lobby_id = lid;

  // Temporary buffer
  char ipvr_str_buf[CLIENT_CONTENTS_SIZE] = "";
  // Copy PUBLIC ipv4 str to temp buffer
  strncpy(ipv4_str_buf, peer_addr_pub.rep_str.c_str(), CLIENT_CONTENTS_SIZE);
  ipv4_str_buf[CLIENT_CONTENTS_SIZE-1] = '\0';
  // Copy to contents buffer
  memcpy(contents, ipv4_str_buf, CLIENT_CONTENTS_SIZE);

  // Copy PRIVATE ipv4 str to temp buffer
  ipv4_str_buf = "";
  strncpy(ipv4_str_buf, peer_addr_priv.rep_str.c_str(), CLIENT_CONTENTS_SIZE);
  ipv4_str_buf[CLIENT_CONTENTS_SIZE-1] = '\0';
  // Copy to contents buffer
  memcpy(contents+CLIENT_CONTENTS_SIZE, ipv4_str_buf, CLIENT_CONTENTS_SIZE);

  contents[SERVER_CONTENTS_SIZE-1] = '\0';

  // Print
  if (ENABLE_CLIENTPACKET_INSPECTION) {
    std::cout << "[Packet] Packing Peer Address:" << std::endl;
    std::cout << "  [Contents] Type: " << packet_type << std::endl;
    std::cout << "  [Contents] SID: " << session_id << std::endl;
    std::cout << "  [Contents] LID: " << lobby_id << std::endl;
    std::cout << "  [Contents] Pub addr: " << peer_addr_pub.rep_str << std::endl;
    std::cout << "  [Contents] Priv addr: " << peer_addr_priv.rep_str << std::endl;
  }
}

std::vector<TYPE_LOBBY_INFO> ServerPacket::parseLobbyList() {
  // Verify good state
  if (session_id > MAX_N_REQUESTED_LOBBIES) {
    return {};
  }
  uint64_t n_lobbies = session_id;

  std::vector<TYPE_LOBBY_INFO> lobby_list = {};

  uint64_t cur_lobby_id = 0;
  std::string cur_lobby_name = "";

  char name_buf[CLIENT_CONTENTS_SIZE] = "";
  unsigned char *id_buf = (unsigned char*)calloc(CLIENT_CONTENTS_SIZE, sizeof(unsigned char));

  for (uint64_t i = 0; i < n_lobbies; ++i) {
    TYPE_LOBBY_INFO cur_lobby;

    // Get pointer to current pair in buffer
    char* contents_idx_ptr = contents + (CLIENT_CONTENTS_SIZE * 2 * i);

    // Populate temp name buffer
    memcpy(name_buf, contents_idx_ptr, CLIENT_CONTENTS_SIZE);
    // Verify good cstr
    name_buf[CLIENT_CONTENTS_SIZE-1] = '\0';
    // Set current lobby name
    cur_lobby_name = name_buf;

    // Copy from char buffer to unsigned char buffer
    memcpy(id_buf, contents_idx_ptr+CLIENT_CONTENTS_SIZE, CLIENT_CONTENTS_SIZE);
    // Unpack ID
    cur_lobby_id = unpacku64(id_buf);

    // Set values
    cur_lobby.first = cur_lobby_name;
    cur_lobby.second = cur_lobby_id;

    // Push to vector
    lobby_list.push_back(cur_lobby);
  }

  free(id_buf);
  return lobby_list;
}

clientAddrInfo ServerPacket::parseAddrInfo() {
  clientAddrInfo peer_addr;
  contents[SERVER_CONTENTS_SIZE-1] = '\0';

  // Temp buffer
  unsigned char addr_buf[sizeof(peer_addr.addr)];
  unsigned char port_buf[sizeof(peer_addr.port)];

  // Copy
  memcpy(addr_buf, contents, sizeof(peer_addr.addr));
  memcpy(port_buf, contents+sizeof(peer_addr.addr), sizeof(peer_addr.port));

  // Host order
  peer_addr.addr = unpacku32(addr_buf);
  peer_addr.port = unpacku16(port_buf);

  // Build repr string
  std::stringstream ss;
  ss << std::setw(3) << (int)(((unsigned char*)&peer_addr.addr)[0]);
  ss << std::setw(1) << ".";
  ss << std::setw(3) << (int)(((unsigned char*)&peer_addr.addr)[1]);
  ss << std::setw(1) << ".";
  ss << std::setw(3) << (int)(((unsigned char*)&peer_addr.addr)[2]);
  ss << std::setw(1) << ".";
  ss << std::setw(3) << (int)(((unsigned char*)&peer_addr.addr)[3]);
  ss << std::setw(1) << ":" << peer_addr.port;
  peer_addr.rep_str = ss.str();

  // Print
  if (ENABLE_CLIENTPACKET_INSPECTION) {
    std::cout << "[Packet] Parsing Peer Address:" << std::endl;
    std::cout << "  [Contents] Type: " << packet_type << std::endl;
    std::cout << "  [Contents] SID: " << session_id << std::endl;
    std::cout << "  [Contents] LID: " << lobby_id << std::endl;
    std::cout << "  [Contents] Addr: " << peer_addr.rep_str << std::endl;
  }
  return peer_addr;
}

/**
 * NET UTILS - courtesy of l'beej
 */

void packi16(unsigned char *buf, unsigned int i)
{
    *buf++ = i>>8; *buf++ = i;
}

void packi32(unsigned char *buf, unsigned long int i)
{
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}

void packi64(unsigned char *buf, uint64_t i)
{
    *buf++ = i>>56; *buf++ = i>>48;
    *buf++ = i>>40; *buf++ = i>>32;
    *buf++ = i>>24; *buf++ = i>>16;
    *buf++ = i>>8;  *buf++ = i;
}

uint16_t unpacku16(unsigned char *buf)
{
    return ((uint16_t)buf[0]<<8) | buf[1];
}

uint32_t unpacku32(unsigned char *buf)
{
    return ((uint32_t)buf[0]<<24) |
           ((uint32_t)buf[1]<<16) |
           ((uint32_t)buf[2]<<8)  |
           buf[3];
}

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
