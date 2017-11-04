#ifndef HEADER_CLIENT
#define HEADER_CLIENT

#include "util.hpp"
#include "tcp.hh"
#include <string>

class FileSyncClient;
class FilesyncAction;

class FilesyncAction {
public:
  FilesyncAction(int type, std::string arg);
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
  void start();
  void close();
  void wait();
  void initdir();
  std::string userid;
private:
  void sync();
  void action_handler();
  bool sync_running;
  std::thread sync_thread;
  std::map<std::string, File> files;
  std::mutex filesmutex;
  TCPClient tcp;
  std::mutex tcpmutex;
  std::string userdir_prefix;
  std::queue<FilesyncAction> actions_queue;
  int connection;    // tcp socket
  bool connected;
};

#endif
