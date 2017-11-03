#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"
#include "tcp.hh"

class FileSyncServer;
class FileSyncSession;
class ConnectedUser;

class FileSyncServer {
friend class FileSyncSession;
public:
  FileSyncServer(void);
  void start();
  FileSyncSession* accept();
  void set_port(int port);
  void set_queue_size(int queue_size);
  void listen();
  void run();
protected:
  TCPServer tcp;
  int max_con;
  int tcp_port;
  int tcp_queue_size;
  bool tcp_active;
  bool running;
  std::map<std::string, ConnectedUser> users;
  std::mutex usersmutex;
  std::list<FileSyncSession*> sessions;
  std::mutex sessionsmutex;
  std::queue<std::string> qlog;
  std::mutex qlogmutex;
  std::queue<std::string> qerrorlog;
  std::mutex qerrorlogmutex;
};

class FileSyncSession {
friend class FileSyncServer;
public:
  void close();
  ~FileSyncSession(void);
  void handle_requests();
protected:
  FileSyncSession(void);
  pthread_t* session;
  TCPConnection* tcp;
  ConnectedUser* user;
  FileSyncServer* server;
  bool active;
};

class ConnectedUser {
friend class FileSyncSession;
public:
  std::string userid;
  ConnectedUser(void);
protected:
  std::list<FileSyncSession*> sessions;
  std::mutex sessionsmutex;
};

#endif
