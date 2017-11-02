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
#include <list>
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

#define LOGIN_ACCEPT     1
#define LOGIN_DENY       2
#define UPLOAD_ACCEPT    3
#define UPLOAD_DENY      4
#define DOWNLOAD_ACCEPT  5
#define NOT_FOUND       44
// structures

#define TRANSFER_OK     10
#define TRANSFER_END    11

#define MSG_LENGTH 512

typedef struct filesync_message {
  uint32_t type;
  char content[MSG_LENGTH];
} fs_message_t;

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

typedef struct filesync_action {
  uint32_t id;
  uint32_t type;
  char name[MAXNAME];
  char extension[MAXNAME];
} fs_action_t;

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
  File(fileinfo_t fileinfo);
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
