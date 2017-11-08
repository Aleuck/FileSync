#ifndef HEADER_SERVER
#define HEADER_SERVER

#include "util.hpp"
#include "tcp.hpp"

class FileSyncServer;
class FileSyncSession;
class ConnectedUser;

class FileSyncServer {
friend class FileSyncSession;
public:
  FileSyncServer(void);
  FileSyncSession* accept();
  void set_port(int port);
  void set_queue_size(int queue_size);
  void listen();
  void start();
  void stop();
  void wait();
protected:
  void run();
  TCPServer tcp;
  unsigned int max_con;
  int tcp_port;
  int tcp_queue_size;
  bool tcp_active;
  bool running;
  std::string homedir;
  std::thread thread;
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
  void handle_login(fs_message_t& msg);
  void handle_logout(fs_message_t& msg);
  void handle_sync(fs_message_t& msg);
  void handle_flist(fs_message_t& msg);
  void handle_upload(fs_message_t& msg);
  void handle_download(fs_message_t& msg);
  void handle_delete(fs_message_t& msg);
protected:
  FileSyncSession(void);
  void _run();
  std::thread thread;
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
