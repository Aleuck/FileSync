#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "data.hpp"

class Server {
public:
  std::map<std::string, User> users;
  std::map<std::string, Client> clients;
};

Server server;

#endif
