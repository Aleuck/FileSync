#ifndef HEADER_TCP
#define HEADER_TCP

#include <string>
#include <sys/types.h>

class TCPServer;
class TCPClient;
class TCPConnection;

class TCPServer {
public:
  TCPServer(void);
  TCPServer(int port);
  TCPConnection* accept();
private:
  struct sockaddr_in addr;
  int sock_d;
};

class TCPClient {
public:
  TCPClient(void);
  TCPConnection* req_con(char* addr, int port);
private:
  struct sockaddr_in addr;
  int sock_d;
};

class TCPConnection {
friend class TCPServer;
friend class TCPClient;
public:
  ssize_t send(char* buffer, size_t length);
  ssize_t recv(char* buffer, size_t length);
  // void lock();
  // void unlock();
private:
  TCPConnection(void);
  struct sockaddr_in addr;
  int sock_d;
};

#endif
