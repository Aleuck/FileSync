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
  clnt.start(); // starts client threads
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
  memset(msg.content, 0, sizeof(MSG_LENGTH));
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
void FileSyncClient::enqueue_action(FilesyncAction action) {
  actions_mutex.lock();
  action.id = ++last_action;
  actions_queue.push(action);
  actions_mutex.unlock();
  actions_sem.post();
}
void FileSyncClient::action_handler() {
  int sig;
  while (running) {
    // wait for an action to be available
    try {
      // semmaphore waits till there are actions to perform
      sig = actions_sem.wait();
    }
    catch (runtime_error e) {
      // some error occured, possibly deadlock condition?
      std::cout << e.what() << '\n';
      exit(-1);
    }
    if (sig) {
      // semaphore interrupted by signal
      exit(0);
    }
    // get next action;
    actions_mutex.lock();
    FilesyncAction action = actions_queue.front();
    actions_queue.pop();
    actions_mutex.unlock();
    switch (action.type) {
      case REQUEST_FLIST:
        list_files(action.arg);
        break;
      case REQUEST_UPLOAD:
        upload_file(action.arg);
        break;
      case REQUEST_DOWNLOAD:
        download_file(action.arg);
        break;
      case REQUEST_DELETE:
        delete_file(action.arg);
        break;
      default:
        log("Action not implemented");
    }
    // execute action
  }
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
  string homedir = get_homedir();
}

void FileSyncClient::log(std::string msg) {
  qlog_mutex.lock();
  qlog.push(msg);
  qlog_mutex.unlock();
  qlog_sem.post();
}


void FileSyncClient::upload_file(std::string filepath) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = REQUEST_UPLOAD;
}
void FileSyncClient::download_file(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = REQUEST_DOWNLOAD;
}
void FileSyncClient::delete_file(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = REQUEST_DELETE;
}
void FileSyncClient::list_files(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = REQUEST_FLIST;
}

FilesyncAction::FilesyncAction(int type, std::string arg) {
  this->type = type;
  this->arg = arg;
}
