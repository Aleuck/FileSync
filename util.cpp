#include "util.hpp"

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
  std::cerr << "semaphore init" << '\n';
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
  return err;
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

std::string filename_from_path(std::string filepath) {
  char cfilepath[MAXNAME];
  strncpy(cfilepath, filepath.c_str(), MAXNAME);
  std::string filename = basename(cfilepath);
  return filename;
}
