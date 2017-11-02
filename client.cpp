#include "client.hpp"
#include "clientUI.hpp"

#define USERDIR_PREFIX "sync_dir_"

using namespace std;

FileSyncClient* client = NULL;

int main(int argc, char* argv[]) {
  cout << "Starting client..." << endl;
  FileSyncClient clnt;
  if (argc < 4) {
    cerr << "Usage: $ ";
    cerr << argv[0] << " <username> <address> <port>" << endl;
    return 1;
  }
  client = &clnt;

  string userid(argv[1]);
  string address(argv[2]);
  int port = atoi(argv[3]);

  try {
    clnt.connect(address, port);
  }
  catch (runtime_error e) {
    cerr << e.what() << endl;
    return 1;
  }
  if (!clnt.login(userid)) {
    std::cerr << "Login failed" << '\n';
  } else {
    std::cerr << "Login Successful" << '\n';
  }
  return 0;
}

FileSyncClient::FileSyncClient(void) {
  userdir_prefix.assign(USERDIR_PREFIX);
}
FileSyncClient::~FileSyncClient(void) {
}
void FileSyncClient::connect(std::string address, int port) {
  tcp.connect(address, port);
}
bool FileSyncClient::login(std::string uid) {
  fs_message_t msg;
  msg.type = htonl(REQUEST_LOGIN);
  memset(msg.content, 0, MSG_LENGTH);
  strncpy(msg.content, uid.c_str(), MAXNAME);
  tcp.send((char*) &msg, sizeof(msg));
  tcp.recv((char*) &msg, sizeof(msg));
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case LOGIN_ACCEPT:
      userid.assign(uid);
      return true;
    case LOGIN_DENY:
      return false;
    default:
      return false;
  }
}
void FileSyncClient::upload_file(std::string filepath) {}
void FileSyncClient::download_file(std::string filename) {}
void FileSyncClient::delete_file(std::string filename) {}
void FileSyncClient::list_files(std::string filename) {}
