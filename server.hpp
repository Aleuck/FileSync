#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"

#define MAX_CON 2
#define QUEUE_SIZE 5

class TCPServer;
class TCPClient;
class TCPConnection;

class TCPServer {
public:
  TCPServer(void);
  TCPServer(int port);
  TCPConnection* accept_con();
private:
  struct sockaddr_in addr;
  int sock_l;
};

class TCPClient {
public:
  TCPClient(void);
};

class TCPConnection {
friend class TCPServer;
friend class TCPServer;
public:
  ssize_t send_bytes(char* buffer, size_t length);
  ssize_t recv_bytes(char* buffer, size_t length);
  // void lock();
  // void unlock();
private:
  TCPConnection(void);
  struct sockaddr_in addr;
  int sock_d;
  std::mutex mutex;
};

#endif
