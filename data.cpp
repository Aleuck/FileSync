#include <cstdlib>
#include <cstring>

#include "data.hpp"

using namespace std;

fileinfo_t File::serialize() {
  fileinfo_t info;
  strcpy(info.name, this.name.c_str());
  strcpy(info.extension, this.extension.c_str());
  strcpy(info.last_modification, this.last_modification.c_str());
  info.size = this.size;
  return info;
}

userinfo_t User::serialize() {
  userinfo_t info;
  strcpy(info.name, this.name.c_str());
  info.id = this.id;
  info.num_files = this.files.size();
  return info;
}

void User::unserialize(userinfo_t info); {
  this.name = info.name;
  this.
}
void File::unserialize(fileinfo_t info); {
  this.owner = info.owner;
  this.name = info.name;
  this.extension = info.extension;
  this.last_modification = info.last_modification;
  this.size = info.size;
}
