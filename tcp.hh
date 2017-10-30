#ifndef HEADER_TCP
#define HEADER_TCP

#include <string>
#include <sys/types.h>
#include <netinet/in.h>

class TCPServer;
class TCPClient;
class TCPConnection;

class TCPSock {
public:
  std::string getAddr();
  int getPort();
protected:
  TCPSock(void);
  struct sockaddr_in addr;
  int sock_d;
};

class TCPServer : public TCPSock {
public:
  TCPServer(void);
  void bind(int port);
  void listen(int queue_size);
  TCPConnection* accept();
};

class TCPConnection : public TCPSock {
friend class TCPServer;
friend class TCPClient;
public:
  ssize_t send(char* buffer, size_t length);
  ssize_t recv(char* buffer, size_t length);
  // void lock();
  // void unlock();
private:
  TCPConnection(void);
  size_t sendbuffer;
  size_t recvbuffer;
  int getbuffersizes();
};

class TCPClient : public TCPConnection {
public:
  TCPClient(void);
  void connect(std::string addr, int port);
};

#endif