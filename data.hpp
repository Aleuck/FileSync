#ifndef HEADER_DATA
#define HEADER_DATA

#include <stdint.h>
#include <string>
#include <cstring>
#include <ctime>
#include <map>
#include <queue>

#define MAXNAME 256

// structures

typedef struct userinfo {
  char name[MAXNAME];
  uint32_t num_files;
} userinfo_t;

typedef struct fileinfo {
  char name[MAXNAME];
  char extension[MAXNAME];
  char last_modification[MAXNAME];
  uint32_t size;
} fileinfo_t;

// classes

class File {
public:
  std::string owner;
  std::string name;
  std::string extension;
  std::string last_modification;
  uint32_t size;
  fileinfo_t serialize();
  void unserialize(fileinfo_t info);
};


class User {
public:
  std::string name;
  std::map<std::string, File> files;
  userinfo_t serialize();
  void unserialize(userinfo_t info);
};

class Transfer {
public:
  File fileinfo;
};

class Client {
public:

};

#endif
