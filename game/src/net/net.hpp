/**
 * Headers for functions relating to rendering which interact with main.cpp
 */

#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <cstring>

#include "engine.hpp"
#include "util.hpp"

constexpr char *SERVER_ADDR = (char*)"192.168.1.100";

struct ClientServerConnectPacket {
  
};

struct P2PConnectInfo {
  std::string peer_addr = "";
  int port = 0;
  int character_id = CHARACTER_ID_HUNKO;
};

int testNetClient();
