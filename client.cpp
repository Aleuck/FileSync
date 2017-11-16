#include "client.hpp"
#include "clientUI.hpp"
#include <sys/inotify.h>
#include <regex>

#define USERDIR_PREFIX "sync_dir_"
#define TEMPFILE_SUFIX ".fstmp"

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
  if (argc >= 5) {
    string dir_prefix(argv[4]);
    clnt.set_dir_prefix(dir_prefix);
  }
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

void FileSyncClient::set_dir_prefix(std::string dir_prefix) {
  userdir_prefix = dir_prefix;
}

bool FileSyncClient::login(std::string uid) {
  fs_message_t msg;
  memset((char*)&msg, 0, sizeof(fs_message_t));
  msg.type = htonl(REQUEST_LOGIN);
  strncpy(msg.content, uid.c_str(), MAXNAME);
  if (!send_message(msg)) return false;
  if (!recv_message(msg)) return false;
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case LOGIN_ACCEPT:
      userid.assign(uid);
      memcpy((char*) &last_update, msg.content, sizeof(uint32_t));
      last_update = ntohl(last_update);
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

void FileSyncClient::initdir() {
  string homedir = get_homedir();
  userdir = homedir + "/" + userdir_prefix + userid;
  create_dir(userdir);
}

void FileSyncClient::log(std::string msg) {
  qlog_mutex.lock();
  qlog.push_front(flocaltime("%T: ", time(NULL)) + msg);
  qlog_mutex.unlock();
  qlog_sem.post();
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
      case REQUEST_SYNC:
        get_update(action.arg);
        break;
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
      case REQUEST_LOGOUT:
        stop();
        break;
      default:
        log("Action not implemented");
        break;
    }
  }
  tcp.close();
}

/* ------------------------------------
   FileSyncClient::sync
    - Watches the userdir for changes
   ------------------------------------ */
void FileSyncClient::sync() {
  struct timespec sleep_time;
  sleep_time.tv_sec = 0;
  sleep_time.tv_nsec = 500000000; // half second

  std::regex ignore_regex("(4913|.*\\.fstmp|.*\\.swpx{0,1}|.*~)$");

  // start inotify
  int fd = inotify_init1(IN_NONBLOCK);
  if (fd < 0) {
    stop();
    return;
  }
  int wd = inotify_add_watch(fd, userdir.c_str(),
    IN_CREATE | IN_DELETE | IN_CLOSE_WRITE | IN_MOVED_TO | IN_MOVED_FROM |
    IN_DONT_FOLLOW | IN_EXCL_UNLINK);
  if (wd < 0) {
    stop();
    return;
  }
  ssize_t len = 0;
  char buffer[IN_BUF_LEN];

  while (sync_running) {
    // read events
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
    }
    ssize_t i = 0;
    while (i < len) {
      struct inotify_event *event;
      event = (struct inotify_event *) &buffer[i];
      std::string filename = event->name;
      std::string filepath = userdir + "/" + filename;
      if(std::regex_match(filename, ignore_regex)) {
        // log("inotify: " + filename + " ignored");
      } else {
        switch (event->mask) {
          case IN_CREATE: // new file
          case IN_MOVED_TO: // moved into folder
          case IN_CLOSE_WRITE: // modified
            log("inotify: upload " + filename);
            enqueue_action(FilesyncAction(REQUEST_UPLOAD, filepath));
            break;
          case IN_DELETE: // deleted
          case IN_MOVED_FROM: // moved from folder
            log("inotify: delete " + filename);
            enqueue_action(FilesyncAction(REQUEST_DELETE, filename));
            break;
        }
      }
      i += IN_EVENT_SIZE + event->len;
    }

    enqueue_action(FilesyncAction(REQUEST_SYNC, ""));
    nanosleep(&sleep_time, NULL);
  }
}

void FileSyncClient::process_update(fs_action_t &update) {
  std::string filename = update.name;
  std::string filepath = userdir + "/" + filename;
  switch (update.type) {
    case A_FILE_UPDATED: {
      enqueue_action(FilesyncAction(REQUEST_DOWNLOAD, filepath));
    } break;
    case A_FILE_DELETED: {
      remove(filepath.c_str());
      files_mtx.lock();
      files.erase(filename);
      files_mtx.unlock();
    } break;
    default: {
      //nothing
    } break;
  }
}

void FileSyncClient::stop() {
  if (sync_running || running) {
    log("exiting...");
    sync_running = false;
    running = false;
    tcp.shutdown();
  }
}

/* ------------------------------------
   FileSyncClient::wait
    - Waits client threads to finish
   ------------------------------------ */
void FileSyncClient::wait() {
  ah_thread.join();
  sync_thread.join();
}

void FileSyncClient::get_update(std::string filepath) {
  // request updates
  fs_message_t msg;
  fs_action_t *update = (fs_action_t *) msg.content;
  memset((char*) &msg, 0, sizeof(fs_message_t));
  msg.type = htonl(REQUEST_SYNC);
  update->id = htonl(last_update);
  if (!send_message(msg)) stop();

  // parse response
  if (!recv_message(msg)) stop();
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case NEW_ACTION:
      update->id = ntohl(update->id);
      update->type = ntohl(update->type);
      update->timestamp = ntohl(update->timestamp);
      update->sid = ntohl(update->sid);
      last_update = update->id;
      process_update(*update);
      break;
    case NO_NEW_ACTION:
      // do nothing
      break;
  }
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
  if (!send_message(msg)) {
    file.close();
    return;
  }
  if (!recv_message(msg)) {
    file.close();
    return;
  }
  // verify response
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case UPLOAD_ACCEPT: {
      // get new file timestamp
      fileinfo_t *new_info = (fileinfo_t*) msg.content;
      fileinfo.last_mod = ntohl(new_info->last_mod);
      fileinfo.size = ntohl(new_info->size);
      // prepare to send file
      msg.type = htonl(TRANSFER_OK);
      file.seekg(0, ios::beg);
      size_t total_sent = 0;
      while (total_sent < fileinfo.size) {
        memset(msg.content, 0, MSG_LENGTH);
        file.read(msg.content, MSG_LENGTH);
        if (file.eof()) {
          msg.type = htonl(TRANSFER_END);
        }
        if (!send_message(msg)) {
          file.close();
          return;
        }
        total_sent += MSG_LENGTH;
      }
      file.close();
      struct utimbuf utimes;
      utimes.actime = fileinfo.last_mod;
      utimes.modtime = fileinfo.last_mod;
      utime(filepath.c_str(), &utimes);

      files_mtx.lock();
      files[filename] = fileinfo;
      files_mtx.unlock();
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
  std::string dirname = dirname_from_path(filepath);
  std::string tmpfilepath = filepath + TEMPFILE_SUFIX;

  std::ofstream file(tmpfilepath, std::ofstream::binary);
  if (!file.is_open()) {
    log("download: Couldn't open file '" + filename + "' to write.");
    return;
  }

  // make request
  msg.type = htonl(REQUEST_DOWNLOAD);
  strncpy(msg.content, filename.c_str(), MAXNAME);
  if (!send_message(msg)) {
    file.close();
    remove(tmpfilepath.c_str());
    return;
  }
  if (!recv_message(msg)) {
    file.close();
    remove(tmpfilepath.c_str());
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
    if (!recv_message(msg)) {
      file.close();
      log("download: connection problem");
      remove(tmpfilepath.c_str());
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
  struct utimbuf utimes;
  utimes.actime = fileinfo.last_mod;
  utimes.modtime = fileinfo.last_mod;
  utime(tmpfilepath.c_str(), &utimes);
  rename(tmpfilepath.c_str(), filepath.c_str());
  files_mtx.lock();
  files[filename] = fileinfo;
  files_mtx.unlock();
}

void FileSyncClient::delete_file(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(fs_message_t));

  msg.type = htonl(REQUEST_DELETE);

  files_mtx.lock();
  files.erase(filename);
  files_mtx.unlock();
  memcpy(msg.content, filename.c_str(), MAXNAME);
  if (!send_message(msg)) return;
  if (!recv_message(msg)) return;
  msg.type = ntohl(msg.type);
}

void FileSyncClient::list_files(std::string filename) {
  fs_message_t msg;
  memset((char*) &msg, 0, sizeof(msg));
  msg.type = htonl(REQUEST_FLIST);

  if (!send_message(msg)) return;
  if (!recv_message(msg)) return;

  uint32_t *net_count = (uint32_t *) msg.content;
  uint32_t count = ntohl(*net_count);

  for (uint32_t i = 0; i < count; ++i) {
    if (!recv_message(msg)) return;
    fileinfo_t *info = (fileinfo_t *) msg.content;
    info->last_mod = ntohl(info->last_mod);
    info->size = ntohl(info->size);
    std::string filename(info->name);
  }
}

bool FileSyncClient::send_message(fs_message_t& msg) {
  ssize_t aux;
  try {
    aux = tcp.send((char*)&msg, sizeof(fs_message_t));
    if (!aux) {
      stop();
      return false;
    }
  }
  catch (std::runtime_error e) {
    log(e.what());
    stop();
    return false;
  }
  return true;
}

bool FileSyncClient::recv_message(fs_message_t& msg) {
  ssize_t aux;
  try {
    aux = tcp.recv((char*)&msg, sizeof(fs_message_t));
    if (!aux) {
      stop();
      return false;
    }
  }
  catch (std::runtime_error e) {
    log(e.what());
    stop();
    return false;
  }
  return true;
}

FilesyncAction::FilesyncAction(int type, std::string arg) {
  this->type = type;
  this->arg = arg;
}
