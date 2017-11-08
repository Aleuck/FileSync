#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"
#include "tcp.hpp"
#include <mutex>

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_MAX_CON 2
#define SERVER_DIR "FileSync_files"

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
  // std::cout << "starting interface..." << std::endl;
  // errCode = pthread_create(&thread_ui, NULL, serverUI, (void*) &serv);
  // if (errCode != 0) {
  //   std::cout << "Error starting interface." << std::endl;
  //   exit(1);
  // }

  // pthread_join(thread_ui, NULL);
  serv.wait();
  exit(0);
}

void close_server(int sig) {
  server->stop();
  server->wait();
  // pthread_kill(thread_ui, sig);
  // pthread_join(thread_ui, NULL);
  // std::cout << "Stopping service..." << sig << std::endl;
  // pthread_kill(thread_server, sig);
  // pthread_join(thread_server, NULL);
}

FileSyncServer::FileSyncServer(void) {
  max_con = DEFAULT_MAX_CON;
  tcp_port = DEFAULT_PORT;
  tcp_queue_size = DEFAULT_QUEUE_SIZE;
  tcp_active = false;
  running = false;
  homedir = get_homedir();
  std::cout << homedir << '\n';
}

void FileSyncServer::start() {
  tcp.bind(tcp_port);
  tcp.listen(tcp_queue_size);
  tcp_active = true;
  running = true;
  thread = std::thread(&FileSyncServer::run, this);
}

void FileSyncServer::stop() {
  running = false;
}

void FileSyncServer::wait() {
  thread.join();
}

void FileSyncServer::run() {
  FileSyncSession* session;
  std::cerr << "running" << '\n';
  while (running) {
    try {
      session = accept();
      //errCode = pthread_create(thread_session, NULL, run_session, (void*) session);
      std::cerr << "connection accepted" << '\n';
      session->thread = std::thread(&FileSyncSession::handle_requests, session);
    }
    catch (std::runtime_error e) {
      std::cerr << e.what() << '\n';
      qlogmutex.lock();
      qlog.push(e.what());
      qlogmutex.unlock();
      continue;
    }
  }
  // server is Stopping
  if (tcp_active) {
    tcp.close();
    tcp_active = false;
  }
}

FileSyncSession* FileSyncServer::accept() {
  FileSyncSession* session = new FileSyncSession;
  TCPConnection* conn = tcp.accept();
  session->tcp = conn;
  session->active = true;
  session->user = NULL;
  session->user = NULL;
  session->server = this;
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
  if (active) {
    close();
  }
  // remove itself from server's session list
  server->sessionsmutex.lock();
  for (auto s = server->sessions.begin(); s != server->sessions.end();) {
    if (*s == this) {
      server->sessions.erase(s);
      break;
    }
  }
  server->sessionsmutex.unlock();
  // remove itself from user's session list
  user->sessionsmutex.lock();
  for (auto s = user->sessions.begin(); s != user->sessions.end();) {
    if (*s == this) {
      user->sessions.erase(s);
    }
  }
  user->sessionsmutex.unlock();
  delete tcp;
}

void FileSyncSession::handle_requests() {
  fs_message_t msg;
  while (active && server->running) {
    memset((char*) &msg, 0, sizeof(msg));
    tcp->recv((char*) &msg, sizeof(msg));
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
  tcp->close();
}

void FileSyncSession::handle_login(fs_message_t& msg) {
  fs_message_t resp;
  msg.content[MAXNAME-1] = 0;
  std::string uid(msg.content);
  std::cerr << "login: " << uid << '\n';
  server->usersmutex.lock();
  std::cerr << "new user" << '\n';
  server->users[uid].userid.assign(uid);
  memset(resp.content, 0, sizeof(resp.content));
  if (server->users[uid].sessions.size() >= server->max_con) {
    resp.type = LOGIN_DENY;
  } else {
    strncpy(resp.content, uid.c_str(), MAXNAME);
    resp.type = LOGIN_ACCEPT;
    server->users[uid].sessions.push_back(this);
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
  memcpy((char*) fileinfo, msg.content, sizeof(fileinfo_t));
  fileinfo.size = ntohl(fileinfo.size);
  fileinfo.last_mod = ntohl(fileinfo.last_mod);
}
void FileSyncSession::handle_download(fs_message_t& msg) {
}
void FileSyncSession::handle_delete(fs_message_t& msg) {
}

void FileSyncSession::close() {
  active = false;
}

ConnectedUser::ConnectedUser(void) {}
