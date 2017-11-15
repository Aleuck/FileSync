#include "client.hpp"
#include "clientUI.hpp"
#include <sys/inotify.h>

#define USERDIR_PREFIX "sync_dir_"
#define TEMPFILE_SUFIX ".temp"

#define IN_EVENT_SIZE (sizeof(struct inotify_event))
#define IN_BUF_LEN (64 * (IN_EVENT_SIZE + 16))
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
    clnt.stop();
    exit(0);
  } else {
    std::cerr << "Logged in successfully as ";
    std::cerr << clnt.userid << '\n';
  }
  clnt.initdir(); // create dir if needed
  clnt.start(); // starts client threads
  // initate interface
  FSClientUI ui(&clnt);
  ui.start();
  clnt.wait(); // wait till client is closed
  ui.close();
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
  running = true;
  sync_thread = std::thread(&FileSyncClient::sync, this);
  ah_thread = std::thread(&FileSyncClient::action_handler, this);
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
  tcp.close();
}

void FileSyncClient::stop() {
  if (sync_running || running) {
    log ("exiting?");
    sync_running = false;
    running = false;
  }
}

/* ------------------------------------
   FileSyncClient::sync
    - Watches the userdir for changes
   ------------------------------------ */
void FileSyncClient::sync() {
  int fd = inotify_init1(IN_NONBLOCK);
  if (fd < 0) {
    stop();
    return;
  }
  int wd = inotify_add_watch(fd, userdir.c_str(),
    IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM |
    IN_DONT_FOLLOW | IN_EXCL_UNLINK);
  if (wd < 0) {
    stop();
    return;
  }
  ssize_t len = 0;
  char buffer[IN_BUF_LEN];
  while (sync_running) {
    len = read(fd, buffer, IN_BUF_LEN);
    if (len < 0) {
      switch (errno) {
        case EAGAIN:
          // no data available, do nothing;
          break;
        case EINTR:
          // interrupted by signal
          break;
        default:
          log("inotify problem");
          break;
      }
      continue;
    }
    ssize_t i = 0;
    while (i < len) {
      struct inotify_event *event;
      event = (struct inotify_event *) &buffer[i];
      log("--i event--");
      log(std::string(event->name));
      if (IN_CREATE & event->mask) log("IN_CREATE");
      if (IN_DELETE & event->mask) log("IN_DELETE");
      if (IN_MODIFY & event->mask) log("IN_MODIFY");
      if (IN_MOVED_TO & event->mask) log("IN_MOVED_TO");
      if (IN_MOVED_FROM & event->mask) log("IN_MOVED_FROM");
      i += IN_EVENT_SIZE + event->len;
    }
  }
}

/* ------------------------------------
   FileSyncClient::wait
    - Waits client threads to finish
   ------------------------------------ */
void FileSyncClient::wait() {
  sync_thread.join();
  ah_thread.join();
}

void FileSyncClient::initdir() {
  string homedir = get_homedir();
  userdir = homedir + "/" + userdir_prefix + userid;
  create_dir(userdir);
}

void FileSyncClient::log(std::string msg) {
  qlog_mutex.lock();
  qlog.push_front(msg);
  qlog_mutex.unlock();
  qlog_sem.post();
}

void FileSyncClient::upload_file(std::string filepath) {
  // try to open and get stats;
  struct stat filestats;
  std::ifstream file(filepath, std::ios::in|std::ios::binary|std::ios::ate);
  int err = stat(filepath.c_str(), &filestats);
  if (!file.is_open() || err) {
    log("upload: Could not open file '" + filepath + "'");
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
      stop();
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
            stop();
            return;
          }
        }
        catch (std::runtime_error e) {
          file.close();
          log(e.what());
          stop();
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
      log("upload: file '" + filename + "' refused by server: " + reason);
    } break;
    default: {
      file.close();
      log("upload: file '" + filename + "' failed. Unrecognized server reply.");
    } break;
  }
}

void FileSyncClient::download_file(std::string filepath) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  std::string filename = filename_from_path(filepath);
  msg.type = htonl(REQUEST_DOWNLOAD);
  std::string tmpfilepath = filepath + TEMPFILE_SUFIX;
  std::ofstream file(tmpfilepath, std::ofstream::binary);
  if (!file.is_open()) {
    log("download: Couldn't open file '" + filename + "' to write.");
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
      remove(tmpfilepath.c_str());
      stop();
      return;
    }
  }
  catch (std::runtime_error e) {
    file.close();
    remove(tmpfilepath.c_str());
    log(e.what());
    return;
  }
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case DOWNLOAD_ACCEPT:
      log("download: file '" + filename + "' accepted.");
      break;
    case NOT_FOUND:
      log("download: File '" + filename + "' not found.");
      file.close();
      remove(tmpfilepath.c_str());
      return;
    default:
      file.close();
      remove(tmpfilepath.c_str());
      log("download: Invalid server response");
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
        remove(tmpfilepath.c_str());
        log("download: Connection closed");
        stop();
        return;
      }
    }
    catch (std::runtime_error e) {
      file.close();
      remove(tmpfilepath.c_str());
      log("download: " + std::string(e.what()));
      stop();
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
        remove(tmpfilepath.c_str());
        log("download: transfer did not succeed.");
        return;
    }
    total_received += MSG_LENGTH;
  }
  file.close();
  rename(tmpfilepath.c_str(), filepath.c_str());
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
    stop(); // disconnected
    return;
  }
  msg.type = ntohl(msg.type);
}

void FileSyncClient::list_files(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = htonl(REQUEST_FLIST);
  ssize_t aux;
  aux = tcp.send((char*) &msg, sizeof(fs_message_t));
  aux = tcp.recv((char*) &msg, sizeof(fs_message_t));
}

FilesyncAction::FilesyncAction(int type, std::string arg) {
  this->type = type;
  this->arg = arg;
}
