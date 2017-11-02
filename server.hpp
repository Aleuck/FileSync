#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"
#include "tcp.hh"

class FileSyncServer;
class FileSyncSession;

class FileSyncServer {
public:
  FileSyncServer(void);
  void start();
  FileSyncSession* accept();
  void set_port(int port);
  void set_queue_size(int queue_size);
  void listen();
  void run();
private:
  TCPServer tcp;
  int max_con;
  int tcp_port;
  int tcp_queue_size;
  bool tcp_active;
  bool running;
  std::queue<std::string> qlog;
  std::map<std::string, User> usersonline;
  std::mutex qlogmutex;
  std::queue<std::string> qerrorlog;
  std::mutex qerrorlogmutex;
};

class FileSyncSession {
friend class FileSyncServer;
public:
  void close();
protected:
  FileSyncSession(void);
  TCPConnection* tcp;
};

class ConnectedUser {
public:
  ConnectedUser(void);
protected:
  std::list<FileSyncSession*> sessions;
};

#endif
