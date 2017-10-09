#ifndef HEADER_CLIENT
#define HEADER_CLIENT

#include "util.hpp"
#include <string>

class Session {
public:
  Session();
  Session(int port);
  ~Session();
  void connect_to_server(std::string address, int port);
  bool login(std::string userid);
  void send_file(std::string filepath);
  void get_file(std::string filename);
  void delete_file(std::string filename);
private:
  std::string userid;
  std::map<std::string, File> files;
  struct sockaddr_in server;
  int connection;    // tcp socket
  pthread_mutex_t mutex;
  bool connected;
};

#endif
