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
  void log(std::string);
  int countSessions();
  int countUsers();
  std::list<std::string> qlog;
  std::mutex qlogmutex;
  Semaphore qlogsemaphore;
  std::list<std::string> qerrorlog;
  std::mutex qerrorlogmutex;
protected:
  void* run();
  bool keep_running;
  bool thread_active;
  std::thread thread;
  // pthread_t pthread;
  TCPServer tcp;
  unsigned int max_con;
  int tcp_port;
  int tcp_queue_size;
  bool tcp_active;
  // clock for time synchronization
  FSClock clock;
  std::string server_dir;
  std::map<std::string, ConnectedUser> users;
  std::mutex usersmutex;
  std::list<FileSyncSession*> sessions;
  std::mutex sessionsmutex;
  // log events
};

class FileSyncSession {
friend class FileSyncServer;
public:
  FileSyncSession(void);
  ~FileSyncSession(void);
  void close();
  void handle_requests();
  void handle_login(fs_message_t& msg);
  void handle_logout(fs_message_t& msg);
  void handle_sync(fs_message_t& msg);
  void handle_flist(fs_message_t& msg);
  void handle_upload(fs_message_t& msg);
  void handle_download(fs_message_t& msg);
  void handle_delete(fs_message_t& msg);
  void logout();
protected:
  uint32_t sid;
  void _run();
  bool keep_running;
  std::thread thread;
  TCPConnection* tcp;
  ConnectedUser* user;
  FileSyncServer* server;
  bool active;
};

class ConnectedUser {
friend class FileSyncSession;
public:
  ConnectedUser(void);
  std::string userid;
  std::string userdir;
protected:
  uint32_t last_sid;
  std::list<FileSyncSession*> sessions;
  std::mutex sessionsmutex;
  std::list<fs_action_t> actions;
  std::mutex actionsmutex;
  uint32_t last_action;
};

#endif
