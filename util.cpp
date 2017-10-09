#include "util.hpp"

using namespace std;

File::File(fileinfo_t fileinfo, User* owner) {

}
fileinfo_t File::serialize() {
  fileinfo_t info;
  strcpy(info.name, this->name.c_str());
  strcpy(info.extension, this->extension.c_str());
  strcpy(info.last_modification, this->last_modification.c_str());
  info.size = this->size;
  return info;
}

void File::unserialize(fileinfo_t info) {
  this->name = info.name;
  this->extension = info.extension;
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
