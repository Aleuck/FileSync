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
  void process_update(fs_action_t &update);
  void get_update(std::string filepath);
  void start();
  void stop();
  void wait();
  void initdir();
  void log(std::string msg);
  bool send_message(fs_message_t& msg);
  bool recv_message(fs_message_t& msg);
  std::string userid;
  std::list<std::string> qlog;
  std::mutex qlog_mutex;
  Semaphore qlog_sem;
private:
  void sync();
  void action_handler();
  std::mutex files_mtx;
  std::map<std::string,fileinfo_t> files;
  std::string userdir_prefix;
  std::string userdir;
  bool sync_running;
  bool running;
  std::thread sync_thread; // thread to watch userdir
  std::thread ah_thread;  // thread to handle actions
  TCPClient tcp;
  uint32_t last_action;
  uint32_t last_update;
  std::queue<FilesyncAction> actions_queue;
  std::mutex actions_mutex;
  Semaphore actions_sem;
  int connection;    // tcp socket
  bool connected;
};

#endif
