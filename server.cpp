#include "server.hpp"
#include "serverUI.hpp"

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_MAX_CON 2
#define SERVER_DIR "Filesync_dir"
#define TEMPFILE_SUFIX ".fstemp"
#define SSL_KEYFILE "KeyFile.pem"
#define SSL_CERTFILE "CertFile.pem"

void close_server(int sig);

// pthread_t thread_ui;

FileSyncServer* server = NULL;

int main(int argc, char* argv[]) {
  FileSyncServer serv;
  server = &serv;
  signal(SIGINT, close_server);
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    exit(0);
  }
  int port = atoi(argv[1]);

  std::cout << "starting service..." << std::endl;
  serv.set_port(port);

  try {
    // Create thread that will run server->run() to handle connection requests
    serv.start(); // async
  }
  catch (std::runtime_error e) {
    std::cerr << e.what() << '\n';
    exit(0);
  }
  // Start interface
  std::cout << "starting interface..." << std::endl;
  FSServerUI ui(&serv);
  ui.start();

  // pthread_join(thread_ui, NULL);
  serv.wait();
  ui.close();
  exit(0);
}

void close_server(int sig) {
  server->stop();
}

FileSyncServer::FileSyncServer(void) {
  max_con = DEFAULT_MAX_CON;
  tcp_port = DEFAULT_PORT;
  tcp_queue_size = DEFAULT_QUEUE_SIZE;
  tcp_active = false;
  thread_active = false;
  keep_running = false;
  is_master = true;
  server_dir = get_homedir() + "/" + SERVER_DIR;
}

// void FileSyncServer::connectToMaster(std::string addr, int port) {
//   //
// }

// FileSyncBackup FileSyncServer::acceptReplic() {
//   //
// }

int FileSyncServer::countSessions() {
  qlogmutex.lock();
  int count = sessions.size();
  qlogmutex.unlock();
  return count;
}

int FileSyncServer::countUsers() {
  usersmutex.lock();
  int count = users.size();
  usersmutex.unlock();
  return count;
}


void FileSyncServer::start() {
  try {
    create_dir(server_dir);
    tcp.open_cert(SSL_CERTFILE, SSL_KEYFILE);
    tcp.bind(tcp_port);
    tcp.listen(tcp_queue_size);
    tcp_active = true;
    keep_running = true;
    thread_active = true;
    thread = std::thread(&FileSyncServer::run, this);
    // aux = pthread_create(&pthread, NULL, (void* (*)(void*)) &FileSyncServer::run, this);
  }
  catch (std::runtime_error e) {
    log(e.what());
  }
}

void FileSyncServer::wait() {
  if (thread_active) {
    thread.join();
    // pthread_join(pthread, NULL);
    thread_active = false;
  }
}

void FileSyncServer::stop() {
  keep_running = false;
  tcp.shutdown();
}

void FileSyncServer::log(std::string msg) {
  qlogmutex.lock();
  qlog.push_front(flocaltime("%T: ", time(NULL)) + msg);
  qlogmutex.unlock();
  qlogsemaphore.post();
}

void* FileSyncServer::run() {
  FileSyncSession* session;
  while (keep_running) {
    session = NULL;
    try {
      session = accept();
    }
    catch (std::runtime_error e) {
      // std::cerr << e.what() << '\n';
      log(e.what());
      continue;
    }
    if (session != NULL) {
      sessionsmutex.lock();
      sessions.push_back(session);
      sessionsmutex.unlock();
      session->thread = std::thread(&FileSyncSession::handle_requests, session);
    }
  }
  // server is Stopping
  tcp.close();
  return NULL;
}

FileSyncSession* FileSyncServer::accept() {
  FileSyncSession* session = NULL;
  try {
    TCPConnection* conn = tcp.accept();
    if (conn == NULL) {
      return NULL;
    }
    session = new FileSyncSession;
    session->tcp = conn;
    session->active = true;
    session->user = NULL;
    session->user = NULL;
    session->server = this;
  }
  catch (std::runtime_error e) {
    throw e;
  }
  fs_message_t msg;
  memset(msg.content, 0, MSG_LENGTH);
  if (is_master) {
    msg.type = htonl(SERVER_GREET);
    session->send_message(msg);
  } else {
    msg.type = htonl(SERVER_REDIR);
    fs_server_t *master_info = (fs_server_t *) msg.content;
    master_info->port = htonl(master_server.port);
    strncpy(master_info->addr, master_server.addr, MAXNAME);
    session->send_message(msg);
  }
  return session;
}

void FileSyncServer::set_port(int port) {
  if (!tcp_active)
    tcp_port = port;
  else
    throw std::runtime_error("Cannot set port while tcp is active");
}

void FileSyncServer::set_queue_size(int queue_size) {
  if (!tcp_active)
    tcp_queue_size = queue_size;
  else
    throw std::runtime_error("Cannot set queue_size while tcp is active");
}


FileSyncSession::FileSyncSession(void) {
  tcp = NULL;
  user = NULL;
  server = NULL;
  rw_sem.post() // initialize the mutex with 1;
}

FileSyncSession::~FileSyncSession(void) {
  delete tcp;
}

void FileSyncSession::logout() {
  log("logged out");
  if (active) {
    close();
  }

  // remove itself from server's session list
  server->sessionsmutex.lock();
  for (auto s = server->sessions.begin(); s != server->sessions.end(); ++s) {
    if (*s == this) {
      // std::cerr << "deleting session from server list" << std::endl;
      server->sessions.erase(s);
      break;
    }
  }
  server->sessionsmutex.unlock();

  server->usersmutex.lock();
  if (user) {
    user->sessionsmutex.lock();
    for (auto s = user->sessions.begin(); s != user->sessions.end(); ++s) {
      if (*s == this) {
        // std::cerr << "deleting session from user list" << std::endl;
        user->sessions.erase(s);
        break;
      }
    }
    user->sessionsmutex.unlock();
  }

  std::string uid = user->userid;
  if (user->sessions.size() == 0) {
    server->users.erase(uid);
  }
  server->usersmutex.unlock();
}

void FileSyncSession::handle_requests() {
  fs_message_t msg;
  while (active && server->keep_running) {
    memset((char*) &msg, 0, sizeof(msg));
    if (!recv_message(msg)){
      break;
    }
    msg.type = ntohl(msg.type);
    switch (msg.type) {
      case REQUEST_LOGIN:
        handle_login(msg);
        break;
      case REQUEST_LOGOUT:
        handle_logout(msg);
        break;
      case REQUEST_SYNC:
        handle_sync(msg);
        break;
      case REQUEST_FLIST:
        handle_flist(msg);
        break;
      case REQUEST_UPLOAD: {
        rw_sem.wait();
        handle_upload(msg);
        rw_sem.post();
      } break;
      case REQUEST_DOWNLOAD:
        handle_download(msg);
        break;
      case REQUEST_DELETE:
        handle_delete(msg);
        break;
    }
  }
  logout();
  tcp->close();
  server = NULL;
  thread.detach();
  delete this;
}

void FileSyncSession::handle_login(fs_message_t& msg) {
  fs_message_t resp;
  memset(resp.content, 0, sizeof(resp.content));

  // get username
  msg.content[MAXNAME-1] = 0;
  std::string uid(msg.content);

  server->usersmutex.lock();
  bool newuser = !server->users.count(uid);
  if (newuser) {
    server->users[uid].userid = uid;
    server->users[uid].userdir = server->server_dir + "/" + uid;
  }
  if (server->users[uid].sessions.size() >= server->max_con) {
    resp.type = LOGIN_DENY;
    log("login: denied user `" + uid + "`");
  } else {
    log("login: accepted user `" + uid + "`");
    resp.type = LOGIN_ACCEPT;
    user = &server->users[uid];
    sid = ++user->last_sid;
    server->users[uid].sessions.push_back(this);

    if (newuser) {
      log("loading user");

      // create userdir if needed
      create_dir(user->userdir);

      // get files already in server dir
      user->files = ls_files(user->userdir);
      if (user->files.size() > 0) {
        log("initial files:");
        for (auto i = user->files.begin(); i != user->files.end(); ++i) {
          log(i->first);
        }
      } else {
        log("server userdir is empty");
      }
    }

    uint32_t last_action = htonl(user->last_action);
    memcpy(resp.content, (char*) &last_action, sizeof(uint32_t));
  }
  server->usersmutex.unlock();
  resp.type = htonl(resp.type);
  send_message(resp);
}

void FileSyncSession::handle_logout(fs_message_t& msg) {
}

void FileSyncSession::handle_sync(fs_message_t& msg) {
  fs_action_t action;
  memcpy((char*) &action, msg.content, sizeof(fs_action_t));
  fs_message_t resp;
  memset((char*) &resp, 0, sizeof(fs_message_t));

  if (!user) {
    resp.type = htonl(UNAUTHENTICATED);
    send_message(resp);
    return;
  }

  action.id = ntohl(action.id);
  action = user->get_next_action(action.id, sid);

  resp.type = htonl(action.id ? NEW_ACTION : NO_NEW_ACTION);

  fs_action_t *resp_ac = (fs_action_t *) resp.content;
  resp_ac->id = htonl(action.id);
  resp_ac->type = htonl(action.type);
  resp_ac->timestamp = htonl(action.timestamp);
  resp_ac->sid = htonl(action.sid);
  strncpy(resp_ac->name, action.name, MAXNAME);

  send_message(resp);
}

void FileSyncSession::handle_flist(fs_message_t& msg) {
  fs_message_t resp;
  memset((char*) &resp, 0, sizeof(fs_message_t));

  if (!user) {
    resp.type = htonl(UNAUTHENTICATED);
    send_message(resp);
    return;
  }

  user->files_mtx.lock();
  uint32_t count = user->files.size();
  uint32_t *net_count = (uint32_t *) resp.content;
  resp.type = htonl(FLIST_ACCEPT);
  *net_count = htonl(count);

  if (!send_message(resp)) {
    user->files_mtx.unlock();
    return;
  }

  resp.type = htonl(TRANSFER_OK);
  fileinfo_t *finfo = (fileinfo_t *) resp.content;
  for (auto ii = user->files.begin(); ii != user->files.end(); ++ii) {
    memset(resp.content, 0, MSG_LENGTH);
    finfo->last_mod = htonl(ii->second.last_mod);
    finfo->size = htonl(ii->second.size);
    strncpy(finfo->name, ii->second.name, MAXNAME);
    if (!send_message(resp)) {
      user->files_mtx.unlock();
      return;
    };
  }
  user->files_mtx.unlock();
}

void FileSyncSession::handle_upload(fs_message_t& msg) {
  fs_message_t resp;
  memset((char*) &resp, 0, sizeof(fs_message_t));

  if (!user) {
    resp.type = htonl(UNAUTHENTICATED);
    send_message(resp);
    return;
  }

  fileinfo_t fileinfo;
  fileinfo_t* old_fileinfo = NULL;
  memcpy((char*) &fileinfo, msg.content, sizeof(fileinfo_t));
  fileinfo.size = ntohl(fileinfo.size);
  fileinfo.last_mod = ntohl(fileinfo.last_mod);
  std::string filename = fileinfo.name;

  std::string filepath = user->userdir + "/" + filename;
  std::string tmpfilepath = filepath + TEMPFILE_SUFIX;

  user->files_mtx.lock();

  std::ofstream file(tmpfilepath, std::ofstream::binary);

  // check existing file
  if (user->files.count(filename)) {
    old_fileinfo = &user->files[filename];
  }
  if (old_fileinfo &&
      old_fileinfo->last_mod == fileinfo.last_mod &&
      old_fileinfo->size     == fileinfo.size) {
    // deny : identical file exists
    log("upload: denied `" + filename + "`.");
    msg.type = htonl(UPLOAD_DENY);
    send_message(msg);
    file.close();
    remove(tmpfilepath.c_str());
    user->files_mtx.unlock();
    return;
  }

  // update file modification time
  fileinfo.last_mod = time(NULL);

  // accept
  msg.type = htonl(UPLOAD_ACCEPT);
  old_fileinfo = (fileinfo_t*) msg.content;
  old_fileinfo->last_mod = htonl(fileinfo.last_mod);
  if (!send_message(msg)) return;
  log("upload: accepted `" + filename + "`.");
  ssize_t total_received = 0;
  while (total_received < fileinfo.size) {
    if (!recv_message(msg)) {
      file.close();
      remove(tmpfilepath.c_str());
      user->files_mtx.unlock();
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
        log("upload: failed `" + filename + "`.");
        user->files_mtx.unlock();
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
  user->files[filename] = fileinfo;

  // register action
  fs_action_t action;
  action.timestamp = fileinfo.last_mod;
  action.type = A_FILE_UPDATED;
  action.sid = sid;
  strncpy(action.name, filename.c_str(), MAXNAME);
  user->log_action(action);

  user->files_mtx.unlock();
  log("upload: finished `" + filename + "`.");
}

void FileSyncSession::handle_download(fs_message_t& msg) {
  fs_message_t resp;
  memset((char*) &resp, 0, sizeof(fs_message_t));

  if (!user) {
    resp.type = htonl(UNAUTHENTICATED);
    send_message(resp);
    return;
  }

  // get file name and path
  msg.content[MAXNAME-1] = 0;
  std::string filename = msg.content;
  std::string filepath = user->userdir + "/" + filename;

  user->files_mtx.lock();
  std::ifstream file(filepath, std::ios::in|std::ios::binary);

  if (!user->files.count(filename) || !file.is_open()) {
    // send 'not found' reply
    resp.type = htonl(NOT_FOUND);
    memcpy(resp.content, msg.content, MSG_LENGTH);
    send_message(resp);

    // check for inconsistency
    if (file.is_open()) {
      file.close();
      remove(filepath.c_str());
    }
    if (user->files.count(filename)) {
      user->files.erase(filename);
    }
  } else {
    // send `accept` reply
    resp.type = htonl(DOWNLOAD_ACCEPT);
    log("download: accepted `"+filename+"`.");
    memset(resp.content, 0, MSG_LENGTH);
    fileinfo_t *hostfileinfo = &user->files[filename];
    fileinfo_t *netfileinfo = (fileinfo_t *) resp.content;
    *netfileinfo = *hostfileinfo;
    netfileinfo->last_mod = htonl(netfileinfo->last_mod);
    netfileinfo->size = htonl(netfileinfo->size);
    send_message(resp);

    // send file content
    resp.type = htonl(TRANSFER_OK);
    size_t total_sent = 0;
    while (total_sent < hostfileinfo->size) {
      memset(resp.content, 0, MSG_LENGTH);
      file.read(resp.content, MSG_LENGTH);
      if (file.eof()) {
        resp.type = htonl(TRANSFER_END);
      }
      if (!send_message(resp)) {
        log("download: failed `" + filename + "`");
        break;
      }
      total_sent += MSG_LENGTH;
    }
    file.close();
  }
  user->files_mtx.unlock();
}

void FileSyncSession::handle_delete(fs_message_t& msg) {
  std::string filename = msg.content;
  std::string filepath = user->userdir + "/" + filename;
  user->files_mtx.lock();
  if (!user->files.count(filename)) {
    log("delete: file not found `" + filename + "`");
    msg.type = htonl(NOT_FOUND);
    remove(filepath.c_str());
    send_message(msg);
  } else {
    remove(filepath.c_str());
    user->files.erase(filename);
    msg.type = htonl(DELETE_ACCEPT);
    send_message(msg);

    // register action
    fs_action_t action;
    action.timestamp = time(NULL);
    action.type = A_FILE_DELETED;
    action.sid = sid;
    strncpy(action.name, filename.c_str(), MAXNAME);
    user->log_action(action);

    server->log("delete: success `" + filename + "`");
  }
  user->files_mtx.unlock();
}

bool FileSyncSession::send_message(fs_message_t& msg) {
  ssize_t aux;
  try {
    aux = tcp->send((char*)&msg, sizeof(fs_message_t));
    if (!aux) {
      close();
      return false;
    }
  }
  catch (std::runtime_error e) {
    log(e.what());
    close();
    return false;
  }
  return true;
}

bool FileSyncSession::recv_message(fs_message_t& msg) {
  ssize_t aux;
  try {
    aux = tcp->recv((char*)&msg, sizeof(fs_message_t));
    if (!aux) {
      close();
      return false;
    }
  }
  catch (std::runtime_error e) {
    log(e.what());
    close();
    return false;
  }
  return true;
}

void FileSyncSession::log(std::string msg) {
  char cssid[64];
  sprintf(cssid,"%d",sid);
  std::string ssid(cssid);
  if (user) {
    server->log("("+user->userid+":"+ssid+") " + msg);
  } else {
    server->log("(~anon) " + msg);
  }
}
void FileSyncSession::close() {
  active = false;
  tcp->close();
}

ConnectedUser::ConnectedUser(void) {
  last_action = 0;
  last_sid = 0;
}

void ConnectedUser::log_action(fs_action_t& action) {
  actions_mtx.lock();
  action.id = ++last_action;
  actions.push_back(action);
  actions_mtx.unlock();
}

fs_action_t ConnectedUser::get_next_action(uint32_t id, uint32_t sid) {
  fs_action_t action;
  memset((char*) &action, 0, sizeof(fs_action_t));
  actions_mtx.lock();
  for (auto i = id; i < actions.size(); ++i) {
    if (actions[i].sid != sid) {
      fs_action_t action = actions[i];
      actions_mtx.unlock();
      return action;
    }
  }
  actions_mtx.unlock();
  return action;
}
