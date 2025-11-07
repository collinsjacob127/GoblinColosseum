
#include "packets.hpp"

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
    std::cout << "[Log] ClientPacket initialized with " 
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
