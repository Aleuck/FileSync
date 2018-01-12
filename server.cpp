#include "server.hpp"
#include "serverUI.hpp"

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_REPL_PORT 53233
#define DEFAULT_MAX_CON 2
#define SERVER_DIR "filesync_dir"
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
  serv.set_port(port);

  if (argc > 3) {
    ServerInfo master;
    master.ip = argv[2];
    master.port = atoi(argv[3]);
    serv.set_master(master);
  }

  std::cout << "starting service..." << std::endl;

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

/* ------------------------ *
    Class: FileSyncServer
 * ------------------------ */

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

void FileSyncServer::connectToMaster(std::string addr, int port) {
  backup_tcp.connect(master.ip, master.port);
  fs_message_t msg;
  fs_server_t *srvinfo = (fs_server_t *) msg.content;
  memset(msg.content, 0. MSG_LENGTH);
  strncpy(srvinfo->ip, addr.c_str(), MAXNAME);
  srvinfo->port = htonl(tcp_port);
  send_msg(backup_tcp, msg);
  recv_msg(backup_tcp, msg);
  msg.type = ntohl(msg.type);
  switch (msg.type) {
    case SERVER_GREET: {} break;
    case SERVER_REDIR: {} break;
    default: {} break;
  }
  master.port = ntohl(srvinfo.port)
}

// FileSyncBackup FileSyncServer::acceptReplic() {
//   //
// }
void FileSyncServer::set_master(ServerInfo server) {
  master = server;
  is_master = false;
}


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
    std::vector<std::string> userdirs = ls_dirs(server_dir);
    for (size_t i = 0; i < userdirs.size(); ++i) {
      users[userdirs[i]].userid = userdirs[i];
      users[userdirs[i]].userdir = server->server_dir + "/" + userdirs[i];
      users[userdirs[i]].files = ls_files(users[userdirs[i]].userdir);
    }
    tcp.open_cert(SSL_CERTFILE, SSL_KEYFILE);
    tcp.bind(tcp_port);
    tcp.listen(tcp_queue_size);
    master_tcp.open_cert(SSL_CERTFILE, SSL_KEYFILE);
    master_tcp.bind(DEFAULT_REPL_PORT);
    master_tcp.listen(tcp_queue_size);
    if (!is_master) {
      connectToMaster(master.ip, master.port);
    }
    tcp_active = true;
    keep_running = true;
    thread_active = true;
    thread = std::thread(&FileSyncServer::run, this);
    master_thread = std::thread(&FileSyncServer::run_master, this);
    if (!is_master) {
      backup_thread = std::thread(&FileSyncServer::run_backup, this);
    }
    // aux = pthread_create(&pthread, NULL, (void* (*)(void*)) &FileSyncServer::run, this);
  }
  catch (std::runtime_error e) {
    log(e.what());
  }
}

void FileSyncServer::wait() {
  if (thread_active) {
    thread.join();
    bkp_thread.join();
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

bool FileSyncServer::connectToMaster

void* FileSyncServer::run_backup() {
}

// method for thread that accept connections from other servers (backups)
void* FileSyncServer::run_master() {
  ServerBackupConn *conn;
  while (keep_running) {
    conn = NULL;
    try {
      conn = accept_bkp();
    } catch (std::runtime_error e) {
      log(e.what());
      continue;
    }
    if (conn != NULL) {
      bkpconnsmutex.lock();
      bkpconns.push_back(conn);
      bkpconnsmutex.unlock();
    }
  }
  master_tcp.close();
}

// method to accept a server connection (backup)
ServerBackupConn* FileSyncServer::accept_bkp() {
  ServerBackupConn* bkp = NULL;

  TCPConnection* conn = master_tcp.accept();
  if (conn == NULL) {
    return NULL;
  }
  bkp = new ServerBackupConn;
  bkp->tcp = conn;
  bkp->info.ip = conn->getAddr();
  bkp->info.port = 0;
  bkp->server = this;

  fs_message_t msg;
  fs_server_t *srvinfo = (fs_server_t *) msg.content;
  memset(msg.content, 0, MSG_LENGTH);
  msg.type = htonl(SERVER_GREET);
  try {
    if (!recv_msg(conn, msg)) {
      conn->close();
      delete bkp;
      return NULL;
    }
  }
  catch (std::runtime_error e) {
    conn->close();
    delete bkp;
    return NULL;
  }
  // get the backup public port
  bkp->info.port = ntohl(srvinfo->port);
  // get my ip address
  if (info.ip = "") {
    info.ip = srvinfo->ip;
  }
  if (is_master) {
    // greet the backup
    memset(msg.content, 0, MSG_LENGTH);
    msg.type = htonl(SERVER_GREET);
    // send back my public port
    srvinfo->port = htonl(tcp_port);
    // send back their ip (so they know it)
    srvinfo->ip = bkp->info.ip;
    send_msg(conn, msg);
  } else {
    // redirect to the current master
    msg.type = htonl(SERVER_REDIR);
    fs_server_t *master_info = (fs_server_t *) msg.content;
    master_info->port = htonl(address.port);
    strncpy(master_info->ip, address.ip.c_str(), MAXNAME);
    send_msg(conn, msg);
    conn->close();
    delete bkp;
    return NULL;
  }
  return bkp;
}

// method for thread that accept connections from clients (sessions)
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

// method to accept a connection from a client (session)
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
    master_info->port = htonl(master.port);
    strncpy(master_info->ip, master.ip.c_str(), MAXNAME);
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

ServerBackupConn::ServerBackupConn(void) {
}

ServerBackupConn::~ServerBackupConn(void) {
  if (tcp) delete tcp;
}

/* ------------------------ *
    Class: FileSyncSession
 * ------------------------ */

FileSyncSession::FileSyncSession(void) {
  tcp = NULL;
  user = NULL;
  server = NULL;
}

FileSyncSession::~FileSyncSession(void) {
  if (tcp) delete tcp;
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

  // remove user if was the only session
  // std::string uid = user->userid;
  // if (user->sessions.size() == 0) {
  //   server->users.erase(uid);
  // }
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
        user->reader_enter();
        handle_sync(msg);
        user->reader_exit();
        break;
      case REQUEST_FLIST:
        user->reader_enter();
        handle_flist(msg);
        user->reader_exit();
        break;
      case REQUEST_UPLOAD: {
        user->writer_enter();
        handle_upload(msg);
        user->writer_exit();
      } break;
      case REQUEST_DOWNLOAD:
        user->reader_enter();
        handle_download(msg);
        user->reader_exit();
        break;
      case REQUEST_DELETE:
        user->writer_enter();
        handle_delete(msg);
        user->writer_exit();
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


  std::ofstream file(tmpfilepath, std::ofstream::binary);

  // check existing file
  user->files_mtx.lock();
  if (user->files.count(filename)) {
    old_fileinfo = &user->files[filename];
  }
  user->files_mtx.unlock();
  if (old_fileinfo &&
      old_fileinfo->last_mod == fileinfo.last_mod &&
      old_fileinfo->size     == fileinfo.size) {
    // deny : identical file exists
    log("upload: denied `" + filename + "`.");
    msg.type = htonl(UPLOAD_DENY);
    send_message(msg);
    file.close();
    remove(tmpfilepath.c_str());
    return;
  }

  // update file modification time
  fileinfo.last_mod = time(NULL);

  // accept, sending the updated timestamp of the file
  msg.type = htonl(UPLOAD_ACCEPT);
  old_fileinfo = (fileinfo_t*) msg.content;
  old_fileinfo->last_mod = htonl(fileinfo.last_mod);
  if (!send_message(msg)) {
    file.close();
    remove(tmpfilepath.c_str());
    return;
  }
  log("upload: accepted `" + filename + "`.");
  try {
    if (!recv_file(tcp, file, fileinfo.size)) {
      file.close();
      remove(tmpfilepath.c_str());
      log("upload: failed `" + filename + "`: disconnected");
      close();
      return;
    }
  }
  catch (std::runtime_error e) {
    file.close();
    remove(tmpfilepath.c_str());
    std::string reason(e.what());
    log("upload: failed `" + filename + "`: " + reason);
    close();
    return;
  }
  file.close();
  struct utimbuf utimes;
  utimes.actime = fileinfo.last_mod;
  utimes.modtime = fileinfo.last_mod;
  utime(tmpfilepath.c_str(), &utimes);

  user->files_mtx.lock();

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

  std::ifstream file(filepath, std::ios::in|std::ios::binary);

  user->files_mtx.lock();
  int aux = user->files.count(filename);
  user->files_mtx.unlock();
  if (!aux || !file.is_open()) {
    // send 'not found' reply
    resp.type = htonl(NOT_FOUND);
    memcpy(resp.content, msg.content, MSG_LENGTH);
    send_message(resp);

    // check for inconsistency
    if (file.is_open()) {
      file.close();
      remove(filepath.c_str());
    }
    user->files_mtx.lock();
    if (user->files.count(filename)) {
      user->files.erase(filename);
    }
    user->files_mtx.unlock();
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
    if (!send_message(resp)) {
      file.close();
      return;
    }
    // send file content
    try {
      if (!send_file(tcp, file, hostfileinfo->size)) {
        file.close();
        log("download: failed `" + filename + "`");
        close();
        return;
      }
    }
    catch (std::runtime_error e) {
      file.close();
      std::string reason(e.what());
      log("download: failed `" + filename + "`: " + reason);
      close();
      return;
    }
    log("download: success `" + filename + "`");
    file.close();
  }
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
  try {
    if (!send_msg(tcp, msg)) {
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
  try {
    if (!recv_msg(tcp, msg)) {
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

/* ------------------------ *
    Class: ConnectedUser
 * ------------------------ */

ConnectedUser::ConnectedUser(void) {
  last_action = 0;
  last_sid = 0;
  read_cont = 0;
  rw_sem.init(1); // initialize the mutex with 1;
  rw_mutex.init(1); // initialize the mutex with 1;
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

void ConnectedUser::reader_enter() {
  rw_mutex.wait();
  read_cont++;
  if (read_cont == 1) {
    rw_sem.wait();
  }
  rw_mutex.post();
}

void ConnectedUser::reader_exit() {
  rw_mutex.wait();
  read_cont--;
  if (read_cont == 0) {
    rw_sem.post();
  }
  rw_mutex.post();
}

void ConnectedUser::writer_enter() {
  rw_sem.wait();
}

void ConnectedUser::writer_exit() {
  rw_sem.post();
}
