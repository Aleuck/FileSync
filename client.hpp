#ifndef HEADER_CLIENT
#define HEADER_CLIENT

#include "util.hpp"
#include "tcp.hpp"
#include <string>

class FileSyncClient;
class FilesyncAction;

class FilesyncAction {
public:
  FilesyncAction(int type, std::string arg);
  uint32_t id;
  int type;
  std::string arg;
protected:
};

class FileSyncClient {
public:
  FileSyncClient(void);
  ~FileSyncClient(void);
  void connect(std::string address, int port);
  bool login(std::string userid);
  void upload_file(std::string filepath);
  void download_file(std::string filename);
  void delete_file(std::string filename);
  void list_files(std::string filename);
  void set_dir_prefix(std::string dir_prefix);
  void enqueue_action(FilesyncAction action);
  void start();
  void close();
  void wait();
  void initdir();
  std::string userid;
  void log(std::string msg);
private:
  void sync();
  void action_handler();
  bool sync_running;
  bool running;
  std::thread sync_thread;
  std::map<std::string, File> files;
  std::mutex filesmutex;
  TCPClient tcp;
  std::mutex tcpmutex;
  std::string userdir_prefix;
  std::queue<FilesyncAction> actions_queue;
  std::mutex actions_mutex;
  uint32_t last_action;
  Semaphore actions_sem;
  std::queue<std::string> qlog;
  std::mutex qlog_mutex;
  Semaphore qlog_sem;
  int connection;    // tcp socket
  bool connected;
};

#endif
