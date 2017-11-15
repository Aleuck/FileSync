#include "server.hpp"
#include "serverUI.hpp"

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_MAX_CON 2
#define SERVER_DIR "Filesync_dir"

void close_server(int sig);

// pthread_t thread_ui;

FileSyncServer* server = NULL;

int main(int argc, char* argv[]) {
  FileSyncServer serv;
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
  server_dir = get_homedir() + "/" + SERVER_DIR;
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
    int aux;
    create_dir(server_dir);
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
  qlog.push_front(flocaltime("%F %T : ", time(NULL)) + msg);
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
}

FileSyncSession::~FileSyncSession(void) {
  delete tcp;
}

void FileSyncSession::logout() {
  if (user) {
    server->log("session of `" + user->userid + "` logging out");
  } else {
    server->log("session logging out");
  }
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
    try {
      ssize_t aux = tcp->recv((char*) &msg, sizeof(msg));
      if (aux == 0) {
        server->log("connection closed.");
        active = false;
        break;
      }
    }
    catch (std::runtime_error e) {
      server->log("problem with connection");
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
      case REQUEST_UPLOAD:
        handle_upload(msg);
        break;
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
  msg.content[MAXNAME-1] = 0;
  std::string uid(msg.content);
  server->log("new login: " + uid);
  server->usersmutex.lock();
  server->users[uid].userid = uid;
  memset(resp.content, 0, sizeof(resp.content));
  if (server->users[uid].sessions.size() >= server->max_con) {
    resp.type = LOGIN_DENY;
  } else {
    strncpy(resp.content, uid.c_str(), MAXNAME);
    resp.type = LOGIN_ACCEPT;
    server->users[uid].sessions.push_back(this);
    user = &server->users[uid];
    sid = ++user->last_sid;
  }
  server->usersmutex.unlock();
  resp.type = htonl(resp.type);
  tcp->send((char*) &resp, sizeof(resp));
}
void FileSyncSession::handle_logout(fs_message_t& msg) {
}
void FileSyncSession::handle_sync(fs_message_t& msg) {
}
void FileSyncSession::handle_flist(fs_message_t& msg) {
}
void FileSyncSession::handle_upload(fs_message_t& msg) {
  fileinfo_t fileinfo;
  memcpy((char*) &fileinfo, msg.content, sizeof(fileinfo_t));
  fileinfo.size = ntohl(fileinfo.size);
  fileinfo.last_mod = ntohl(fileinfo.last_mod);
}
void FileSyncSession::handle_download(fs_message_t& msg) {
  fileinfo_t fileinfo;
  // get file name
  char cfilename[MAXNAME];
  strncpy(cfilename, msg.content, MAXNAME);
  std::string filename = cfilename;
}
void FileSyncSession::handle_delete(fs_message_t& msg) {
}

void FileSyncSession::close() {
  active = false;
  tcp->close();
}

ConnectedUser::ConnectedUser(void) {
  last_action = 0;
  last_sid = 0;
}
