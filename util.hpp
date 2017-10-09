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

// structures

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
  File(fileinfo_t fileinfo, User* owner);
  std::string name;
  std::string extension;
  std::string last_modification;
  User* owner;
  uint32_t size;
  fileinfo_t serialize();
  void unserialize(fileinfo_t info);
};


class Transfer {
public:
  File* fileinfo;
};

#endif
