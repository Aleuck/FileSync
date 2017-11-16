#ifndef HEADER_CLIENTUI
#define HEADER_CLIENTUI

#include "userInterface.hpp"

class FSClientUI;
class FSClientUI : public FilesyncUI {
public:
  FSClientUI(void);
  FSClientUI(FileSyncClient *client);
  void start();
  void close();
protected:
  FileSyncClient *client;
  std::thread thread;
  pthread_t pthread;
  bool running;
  void run();
};


#endif
