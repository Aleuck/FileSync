#ifndef HEADER_CLIENT
#define HEADER_CLIENT

#include "util.hpp"
#include <string>

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
private:
  TCPClient tcp;
  std::string userid;
  std::map<std::string, File> files;
  struct sockaddr_in server;
  int connection;    // tcp socket
  pthread_mutex_t mutex;
  bool connected;
};

#endif
