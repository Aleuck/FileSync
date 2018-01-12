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
#include <unistd.h>
#include <pwd.h>
#include <libgen.h>
#include <utime.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdexcept>
#include <thread>
#include <mutex>
#include <map>
#include <queue>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>

#include "tcp.hpp"

#define MAXNAME 256

#define REQUEST_LOGIN    1
#define REQUEST_LOGOUT   2
#define REQUEST_SYNC     3
#define REQUEST_FLIST    4
#define REQUEST_UPLOAD   5
#define REQUEST_DOWNLOAD 6
#define REQUEST_DELETE   7
#define REQUEST_LOCK    10
#define REQUEST_UNLOCK  11

#define NEW_USER          3
#define NEW_UPDATE        4
#define REQUEST_USERLIST  5
#define REQUEST_USERFILES 6

#define LOGIN_ACCEPT     1
#define LOGIN_DENY       2
#define UPLOAD_ACCEPT    3
#define UPLOAD_DENY      4
#define DOWNLOAD_ACCEPT  5
#define DELETE_ACCEPT    6
#define FLIST_ACCEPT     7
#define NEW_ACTION       8
#define NO_NEW_ACTION    9
#define LOCK_GRANTED    10
#define LOCK_BUSY       11
#define NOT_FOUND       44

#define SERVER_GREET    50
#define SERVER_REDIR    51

#define A_FILE_UPDATED   1
#define A_FILE_DELETED   2

#define ABORT           55
#define UNAUTHENTICATED 66
// structures

#define TRANSFER_OK     10
#define TRANSFER_END    11

#define MSG_LENGTH 1024

class ServerInfo {
public:
  std::string ip;
  int port;
};

typedef struct filesync_message {
  uint32_t type;
  char content[MSG_LENGTH];
} fs_message_t;

typedef struct server {
  uint32_t port;
  char ip[MAXNAME];
} fs_server_t;

typedef struct user {
  char userid[MAXNAME];
  uint32_t num_files;
} user_t;

typedef struct fileinfo {
  char name[MAXNAME];
  uint32_t last_mod;
  uint32_t size;
} fileinfo_t;

typedef struct filesync_action {
  uint32_t id;
  uint32_t type;
  uint32_t timestamp;
  uint32_t sid;
  char name[MAXNAME];
} fs_action_t;

typedef struct user_update {
  char uid[MAXNAME];
  fs_action_t info;
} fs_update_t;

// classes
class User;
class File;
class Transfer;
class Semaphore;
class FSClock;

class User {
public:
  std::string name;
  std::map<std::string, File*> files;
  user_t serialize();
  void unserialize(user_t info);
};

class File {
public:
  File(void);
  File(fileinfo_t fileinfo);
  std::string name;
  time_t last_mod;
  uint32_t size;
  fileinfo_t serialize();
  void unserialize(fileinfo_t info);
};


class Transfer {
public:
  File fileinfo;
  uint32_t size_due;
};

class Semaphore {
public:
  Semaphore(void);
  Semaphore(int value);
  ~Semaphore(void);
  void init(int value);
  int post();
  int wait();
  /* -------------------
  Semaphore::trywait() throws std::runtime_error
  returns 1 if semaphore was already locked
  returns 0 if semaphore was successfully locked
  returns -1 if function was interrupted by signal
  ---------------------- */
  int trywait();
  int destroy();
private:;
  bool initialized;
  sem_t sem;
};

class FSClock {
public:
  FSClock(void);
  uint32_t gettime();
private:
  uint32_t offset;
};
std::string get_homedir();
std::string filename_from_path(std::string filepath);
std::string dirname_from_path(std::string filepath);
std::map<std::string, fileinfo_t> ls_files(std::string dirname);
std::vector<std::string> ls_dirs(std::string dirname);
std::string flocaltime(std::string format, time_t t);
void create_dir(std::string path);
bool cp(std::string from_path, std::string to_path);
bool send_msg(TCPConnection *conn, fs_message_t &msg);
bool recv_msg(TCPConnection *conn, fs_message_t &msg);
bool send_file(TCPConnection *conn, std::ifstream &file, size_t filesize);
bool recv_file(TCPConnection *conn, std::ofstream &file, size_t filesize);
#endif
