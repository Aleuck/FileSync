#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"
#include "tcp.hh"

class FSServer {
public:
  FSServer(void);
  void start();
private:
  TCPServer tcp;
  int port;
  int tcp_queue_size;
  int max_con;
  bool is_running;
  std::queue<std::string> qlog;
  std::mutex qlogmutex;
  std::queue<std::string> qerrorlog;
  std::mutex qerrorlogmutex;
};

#endif
