#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"

class Server {
public:
  std::map<std::string, User> users;
  std::map<std::string, Client> clients;
};

Server server;

#endif
