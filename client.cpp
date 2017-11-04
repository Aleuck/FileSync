#include "client.hpp"
#include "clientUI.hpp"

#define USERDIR_PREFIX "sync_dir_"

using namespace std;

FileSyncClient* client = NULL;

int main(int argc, char* argv[]) {
  cout << "Starting client..." << endl;
  FileSyncClient clnt;
  if (argc < 4) {
    cerr << "Usage: $ " << argv[0];
    cerr << " <username> <address> <port>" << endl;
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
    clnt.close();
  } else {
    std::cerr << "Logged in successfully as ";
    std::cerr << clnt.userid << '\n';
  }
  clnt.initdir(); // create dir if needed
  clnt.start(); //
  clnt.wait(); // wait till client is closed
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
  memset(msg.content, 0, sizeof(msg.content));
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

void FileSyncClient::start() {
  sync_running = true;
  sync_thread = std::thread(&FileSyncClient::sync, this);
}
void FileSyncClient::action_handler() {
}

void FileSyncClient::close() {
  sync_running = false;
  sync_thread.join();
  tcp.close();
}
void FileSyncClient::sync() {
  while (sync_running) {

  }
}
void FileSyncClient::wait() {
  sync_thread.join();
}

void FileSyncClient::initdir() {
}


void FileSyncClient::upload_file(std::string filepath) {}
void FileSyncClient::download_file(std::string filename) {}
void FileSyncClient::delete_file(std::string filename) {}
void FileSyncClient::list_files(std::string filename) {}

FilesyncAction::FilesyncAction(int type, std::string arg) {}
