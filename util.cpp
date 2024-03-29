#include "util.hpp"
// #include <sys/sendfile.h>  // sendfile
#include <fcntl.h>         // open
#include <unistd.h>        // close
#include <sys/stat.h>      // fstat
#include <sys/types.h>     // fstat
#include <dirent.h>
using namespace std;

File::File(fileinfo_t fileinfo) {

}
fileinfo_t File::serialize() {
  fileinfo_t info;
  strcpy(info.name, this->name.c_str());
  info.last_mod = this->last_mod;
  info.size = this->size;
  return info;
}

void File::unserialize(fileinfo_t info) {
  this->name = info.name;
  this->last_mod = info.last_mod;
  this->size = info.size;
}

user_t User::serialize() {
  user_t info;
  strcpy(info.userid, name.c_str());
  info.num_files = files.size();
  return info;
}

void User::unserialize(user_t info) {
  this->name = info.userid;
}

Semaphore::Semaphore(int value) {
  init(value);
}

Semaphore::Semaphore(void) {
  init(0);
}

void Semaphore::init(int value) {
  if (initialized) {
    while (destroy());
  }
  sem_init(&sem, 0, value);
  initialized = true;
}

Semaphore::~Semaphore(void) {
  if (destroy() != 0) {
    std::cerr << "ERROR: Destroyed semaphore in use." << '\n';
    exit(-1);
  }
}

int Semaphore::wait() {
  int err = sem_wait(&sem);
  if (err) switch (errno) {
    case EDEADLK:
      throw runtime_error("Deadlock condition.");
    case EINTR:
      // a signal interrupted the function
      return -1;
    case EINVAL:
      throw runtime_error("Invalid semaphore.");
    default:
      throw runtime_error("Undefined sempaphore error.");
  }
  return err;
}


int Semaphore::trywait() {
  int err = sem_trywait(&sem);
  if (err) switch (errno) {
    case EAGAIN:
      // semaphre already locked
      return 1;
    case EDEADLK:
      throw runtime_error("Deadlock condition.");
    case EINTR:
      // a signal interrupted the function
      return -1;
    case EINVAL:
      throw runtime_error("Invalid semaphore.");
    default:
      throw runtime_error("Undefined sempaphore error.");
  }
  return 0;
}


int Semaphore::post() {
  int err = sem_post(&sem);
  if (err) switch (errno) {
    case EINVAL:
      throw runtime_error("Invalid semaphore.");
    default:
      throw runtime_error("Undefined sempaphore error.");
  }
  return err;
}

int Semaphore::destroy() {
  if (!initialized) return 0;
  int err = sem_destroy(&sem);
  if (err) {
    switch (errno) {
      case EINVAL:
        throw runtime_error("Invalid semaphore.");
      case ENOSYS:
        throw runtime_error("The function sem_destroy() is not supported by this implementation.");
      case EBUSY:
        return -1;
      default:
        throw runtime_error("Undefined sempaphore error.");
    }
  } else {
    initialized = false;
  }
  return err;
}

FSClock::FSClock(void) {
  // This will be used later for time synchronization
  // between different machines
  offset = 0;
}

uint32_t FSClock::gettime(void) {
  return ::time(NULL) + offset;
}

std::string get_homedir() {
  const char* chomedir;
  if ((chomedir = getenv("HOME")) == NULL) {
      std::cerr << "Could not get home dir." << '\n';
      exit(1);
  }
  std::string homedir = chomedir;
  return homedir;
}

void create_dir(std::string path) {
  struct stat st = {0};
  if (stat(path.c_str(), &st) == 0) return;
  if (mkdir(path.c_str(), S_IRWXU | S_IRWXG)) {
    switch (errno) {
      case EACCES:
        throw std::runtime_error("Search permission is denied on a component of the path prefix, or write permission is denied on the parent directory of the directory to be created.");
        break;
      case EEXIST:
        throw std::runtime_error("The named file exists.");
        break;
      case ELOOP:
        throw std::runtime_error("A loop exists in symbolic links encountered during resolution of the path argument.");
        break;
      case EMLINK:
        throw std::runtime_error("The link count of the parent directory would exceed {LINK_MAX}.");
        break;
      case ENAMETOOLONG:
        throw std::runtime_error("The length of the path argument exceeds {PATH_MAX} or a pathname component is longer than {NAME_MAX}.");
        break;
      case ENOENT:
        throw std::runtime_error("A component of the path prefix specified by path does not name an existing directory or path is an empty string.");
        break;
      case ENOSPC:
        throw std::runtime_error("The file system does not contain enough space to hold the contents of the new directory or to extend the parent directory of the new directory.");
        break;
      case ENOTDIR:
        throw std::runtime_error("A component of the path prefix is not a directory.");
        break;
      case EROFS:
        throw std::runtime_error("The parent directory resides on a read-only file system.");
        break;
      default:
        throw std::runtime_error("Unknown error.");
    }
  }
}

bool cp(std::string from_path, std::string to_path) {
  int err = link(from_path.c_str(), to_path.c_str());
  if (err == 0) return true;
  return false;
}

std::string filename_from_path(std::string filepath) {
  char cfilepath[MAXNAME];
  strncpy(cfilepath, filepath.c_str(), MAXNAME);
  std::string filename = basename(cfilepath);
  return filename;
}

std::string dirname_from_path(std::string filepath) {
  char cfilepath[MAXNAME];
  strncpy(cfilepath, filepath.c_str(), MAXNAME);
  std::string dirname_ = dirname(cfilepath);
  return dirname_;
}
std::vector<std::string> ls_dirs(std::string dirname){
  std::vector<std::string> dirs;
  DIR* dirp = opendir(dirname.c_str());
  struct dirent entry;
  struct dirent *result;
  readdir_r(dirp, &entry, &result);
  while (result) {
    std::string entry_name(entry.d_name);
    std::string entry_path = dirname + "/" + entry_name;
    struct stat path_stat;
    stat(entry_path.c_str(), &path_stat);
    if (S_ISDIR(path_stat.st_mode)) {
      if (entry_name != "." && entry_name != "..")
      dirs.push_back(entry_name);
    }
    readdir_r(dirp, &entry, &result);
  }
  return dirs;
}
std::map<std::string, fileinfo_t> ls_files(std::string dirname) {
  std::map<std::string, fileinfo_t> files;
  DIR* dirp = opendir(dirname.c_str());
  struct dirent entry;
  struct dirent *result;
  readdir_r(dirp, &entry, &result);
  while (result) {
    std::string filename(entry.d_name);
    std::string filepath = dirname + "/" + filename;
    struct stat path_stat;
    stat(filepath.c_str(), &path_stat);
    if (S_ISREG(path_stat.st_mode)) {
      fileinfo_t info;
      info.last_mod = path_stat.st_mtime;
      info.size = path_stat.st_size;
      strncpy(info.name, entry.d_name, MAXNAME);
      files[filename] = info;
    }
    readdir_r(dirp, &entry, &result);
  }
  return files;
}

bool send_msg(TCPConnection *conn, fs_message_t &msg) {
  ssize_t aux;
  aux = conn->send((char*)&msg, sizeof(fs_message_t));
  if (!aux) {
    return false;
  }
  return true;
}
bool recv_msg(TCPConnection *conn, fs_message_t &msg) {
  ssize_t aux;
  aux = conn->recv((char*)&msg, sizeof(fs_message_t));
  if (!aux) {
    return false;
  }
  return true;
}
bool send_file(TCPConnection *conn, std::ifstream &file, size_t filesize) {
  fs_message_t msg;
  msg.type = htonl(TRANSFER_OK);
  size_t total_sent = 0;
  while (total_sent < filesize) {
    memset(msg.content, 0, MSG_LENGTH);
    file.read(msg.content, MSG_LENGTH);
    if (file.eof()) {
      msg.type = htonl(TRANSFER_END);
    }
    if (!send_msg(conn, msg)) {
      return false;
    }
    total_sent += MSG_LENGTH;
  }
  return true;
}
bool recv_file(TCPConnection *conn, std::ofstream &file, size_t filesize) {
  fs_message_t msg;
  size_t total_received = 0;
  while (total_received < filesize) {
    if (!recv_msg(conn, msg)) {
      return false;
    }
    msg.type = ntohl(msg.type);
    switch (msg.type) {
      case TRANSFER_OK:
        file.write(msg.content, MSG_LENGTH);
        break;
      case TRANSFER_END:
        if (filesize - total_received > MSG_LENGTH) {
          // security measure
          return false;
        }
        file.write(msg.content, filesize - total_received);
        break;
      default:
        return false;
    }
    total_received += MSG_LENGTH;
  }
  return true;
}

std::string flocaltime(std::string format, time_t t) {
  struct tm tm;
  // using localtime_r() instead of localtime()
  // localtime() is not thread safe
  // but localtime_r() is not portable
  localtime_r(&t, &tm);
  char formated[MAXNAME];
  strftime (formated, MAXNAME, format.c_str(), &tm);
  std::string fdate = formated;
  return fdate;
}
