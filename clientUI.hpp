#ifndef HEADER_CLIENTUI
#define HEADER_CLIENTUI

#include "userInterface.hpp"

class FSClientUI;
class FSClientUI : public FilesyncUI {
public:
  FSClientUI(FileSyncClient *client);
  void start();
  void close();
protected:
  FileSyncClient *client;
  std::thread thread;
  pthread_t pthread;
  bool running;
  void run();
  void exec_cmd(std::string cmd);
  void output(std::string cmd);
};


#endif
