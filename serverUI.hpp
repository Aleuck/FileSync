#ifndef HEADER_SERVERUI
#define HEADER_SERVERUI

#include "userInterface.hpp"

class FSServerUI;
class FSServerUI : public FilesyncUI {
public:
  FSServerUI(FileSyncServer* server);
  void start();
  void close();
protected:
  FileSyncServer* server;
  std::thread thread;
  bool running;
  void run();
};

#endif
