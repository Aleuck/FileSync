#ifndef HEADER_UTIL
#define HEADER_UTIL

#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>
#include <mutex>
#include <map>
#include <queue>
#include <vector>
#include <string>
#include <iostream>

#define MAXNAME 256

#define REQUEST_LOGIN    1
#define REQUEST_LOGOUT   2
#define REQUEST_SYNC     3
#define REQUEST_FLIST    3
#define REQUEST_UPLOAD   4
#define REQUEST_DOWNLOAD 5
#define REQUEST_DELETE   6
// structures

typedef struct filesync_request {
  uint32_t type;
} fs_request_t;

typedef struct user {
  char userid[MAXNAME];
  uint32_t num_files;
} user_t;

typedef struct fileinfo {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modification[MAXNAME];
  uint32_t size;
} fileinfo_t;

typedef struct file_sync_action {
  uint32_t id;
  uint32_t type;
  char name[MAXNAME];
  char extension[MAXNAME];
}

// classes
class User;
class File;
class Transfer;

class User {
public:
  std::string name;
  std::map<std::string, File*> files;
  user_t serialize();
  void unserialize(user_t info);
};

class File {
public:
  File();
  File(fileinfo_t fileinfo;
  std::string name;
  std::string extension;
  std::string last_modification;
  uint32_t size;
  fileinfo_t serialize();
  void unserialize(fileinfo_t info);
};


class Transfer {
public:
  File fileinfo;
  uint32_t size_due;
};



#endif
