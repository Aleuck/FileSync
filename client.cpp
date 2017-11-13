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
    exit(0);
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
      log("Log in accepted.");
      return true;
    case LOGIN_DENY:
      log("Log in denied.");
      return false;
    default:
      log("Could not login. Server error.");
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
  if (sync_running) {
  sync_running = false;
  running = false;
  tcp.close();
  sync_thread.join();
  } else {
    tcp.close();
  }
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
  std::cout << "homedir: " + homedir << '\n';
}

void FileSyncClient::log(std::string msg) {
  qlog_mutex.lock();
  qlog.push(msg);
  qlog_mutex.unlock();
  qlog_sem.post();
}

void FileSyncClient::upload_file(std::string filepath) {
  // try to open and get stats;
  struct stat filestats;
  std::ifstream file(filepath, std::ios::in|std::ios::binary|std::ios::ate);
  int err = stat(filepath.c_str(), &filestats);
  if (!file.is_open() || err) {
    log("Could not open file '" + filepath + "'");
    return;
  }
  // get filename from path
  std::string filename = filename_from_path(filepath);
  // prepare file info
  fileinfo_t fileinfo;
  strncpy(fileinfo.name, filename.c_str(), MAXNAME);
  size_t size = file.tellg();
  fileinfo.last_mod = htonl(filestats.st_mtime);
  fileinfo.size = htonl(size);
  // prepare request message
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(fs_message_t));
  msg.type = htonl(REQUEST_UPLOAD);
  memcpy(msg.content, (char*) &fileinfo, sizeof(fileinfo_t));
  // send message and get reply
  try {
    ssize_t aux;
    aux = tcp.send((char*) &msg, sizeof(fs_message_t));
    aux = tcp.recv((char*) &msg, sizeof(fs_message_t));
    if (aux == 0) {
      close();
      return;
    }
  }
  catch (std::runtime_error e) {
    log(e.what());
    return;
  }
  // verify response
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case UPLOAD_ACCEPT: {
      // prepare to send file
      msg.type = htonl(TRANSFER_OK);
      file.seekg(0, ios::beg);
      size_t total_sent = 0;
      while (!file.eof()) {
        file.read(msg.content, MSG_LENGTH);
        if (file.eof()) {
          msg.type = htonl(TRANSFER_END);
        }
        try {
          ssize_t sent = tcp.send((char*) &msg, sizeof(fs_message_t));
          if (sent == 0) {
            file.close();
            close();
            return;
          }
        }
        catch (std::runtime_error e) {
          file.close();
          log(e.what());
          close();
          return;
        }
        total_sent += MSG_LENGTH;
      }
      file.close();
    } break;
    case UPLOAD_DENY: {
      // log incident and abort
      file.close();
      std::string reason = msg.content;
      log("Upload of '" + filename + "' refused by server: " + reason);
    } break;
    default: {
      file.close();
      log("Upload of '" + filename + "' failed. Unrecognized server reply.");
    } break;
  }
}
void FileSyncClient::download_file(std::string filepath) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  std::string filename = filename_from_path(filepath);
  msg.type = htonl(REQUEST_DOWNLOAD);
  std::ofstream file(filepath, std::ofstream::binary);
  if (!file.is_open()) {
    log("Couldn't open file '" + filename + "' to write.");
    return;
  }
  msg.type = htonl(REQUEST_DOWNLOAD);
  strncpy(msg.content, filename.c_str(), MAXNAME);
  try {
    ssize_t aux;
    aux = tcp.send((char*) &msg, sizeof(fs_message_t));
    aux = tcp.recv((char*) &msg, sizeof(fs_message_t));
    if (aux == 0) {
      file.close();
      close();
      return;
    }
  }
  catch (std::runtime_error e) {
    file.close();
    log(e.what());
    return;
  }
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case DOWNLOAD_ACCEPT:
      log("Downloading file '" + filename + "'.");
      break;
    case NOT_FOUND:
      log("File '" + filename + "' not found.");
      return;
    default:
      log("Invalid server response");
      return;
  }
  fileinfo_t fileinfo;
  memcpy((char*) &fileinfo, msg.content, sizeof(fileinfo_t));
  fileinfo.size = ntohl(fileinfo.size);
  fileinfo.last_mod = ntohl(fileinfo.last_mod);
  size_t total_received = 0;
  while (total_received < fileinfo.size) {
    try {
      ssize_t received;
      received = tcp.recv((char*) &msg, sizeof(fs_message_t));
      if (received == 0) {
        file.close();
        log("Connection closed");
        close();
        return;
      }
    }
    catch (std::runtime_error e) {
      file.close();
      log(e.what());
      close();
      return;
    }
    msg.type = ntohl(msg.type);
    switch (msg.type) {
      case TRANSFER_OK:
        file.write(msg.content, MSG_LENGTH);
        break;
      case TRANSFER_END:
        file.write(msg.content, fileinfo.size - total_received);
        break;
      default:
        file.close();
        log("Invalid message.");
        return;
    }
    total_received += MSG_LENGTH;
  }
  file.close();
}
void FileSyncClient::delete_file(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = htonl(REQUEST_DELETE);
  memcpy(msg.content, filename.c_str(), MAXNAME);
  ssize_t aux;
  aux = tcp.send((char*) &msg, sizeof(fs_message_t));
  aux = tcp.recv((char*) &msg, sizeof(fs_message_t));
  if (aux == 0) {
    close(); // disconnected
    return;
  }
  msg.type = ntohl(msg.type);
}
void FileSyncClient::list_files(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
}

FilesyncAction::FilesyncAction(int type, std::string arg) {
  this->type = type;
  this->arg = arg;
}
