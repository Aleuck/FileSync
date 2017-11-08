#include "util.hpp"

using namespace std;

File::File(fileinfo_t fileinfo) {

}
fileinfo_t File::serialize() {
  fileinfo_t info;
  strcpy(info.name, this->name.c_str());
  info.last_modification = this->last_modification;
  info.size = this->size;
  return info;
}

void File::unserialize(fileinfo_t info) {
  this->name = info.name;
  this->last_modification = info.last_modification;
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
  sem_init(&sem, 0, value);
  initialized = true;
}

Semaphore::Semaphore(void) {
  Semaphore(0);
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

const char* get_homedir() {
  const char* homedir;
  if ((homedir = getenv("HOME")) == NULL) {
      std::cerr << "Could not get home dir." << '\n';
      exit(1);
  }
  return homedir;
}

std::string filename_from_path(std::string filepath) {
  char cfilepath[MAXNAME];
  strncpy(cfilepath, filepath.c_str(), MAXNAME);
  std::string filename = basename(cfilepath);
  return filename;
}
