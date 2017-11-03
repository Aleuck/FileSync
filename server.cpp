#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"
#include "tcp.hh"
#include <mutex>

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_MAX_CON 2

void* run_server(void* arg);
void* run_session(void* arg);
void close_server(int sig);

pthread_t thread_ui;
pthread_t thread_server;

FileSyncServer* server = NULL;

int main(int argc, char* argv[]) {
  int errCode;
  FileSyncServer serv;
  signal(SIGINT, close_server);
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
    exit(0);
  }
  server = &serv;
  int port = atoi(argv[1]);

  std::cout << "starting service..." << std::endl;
  try {
    serv.set_port(port);
    serv.listen();
  }
  catch (std::runtime_error e) {
    std::cerr << e.what() << '\n';
    exit(0);
  }
  errCode = pthread_create(&thread_server, NULL, run_server, (void*) &serv);

  std::cout << "starting interface..." << std::endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, (void*) &serv);
  if (errCode != 0) {
    std::cout << "Error starting interface." << std::endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  pthread_join(thread_server, NULL);
  exit(0);
}

void* run_server(void* arg) {
  int errCode;
  FileSyncServer* serv = (FileSyncServer*) arg;
  serv->run();
  return NULL;
}
void* run_session(void* arg) {
  FileSyncSession* session = (FileSyncSession*) arg;
  session->handle_requests();
  // session stopped handling requests
  delete session;
  return NULL;
}

void close_server(int sig) {
  pthread_kill(thread_ui, sig);
  pthread_join(thread_ui, NULL);
  std::cout << "Stopping service..." << sig << std::endl;
  pthread_kill(thread_server, sig);
  pthread_join(thread_server, NULL);
  exit(0);
}




FileSyncServer::FileSyncServer(void) {
  max_con = DEFAULT_MAX_CON;
  tcp_port = DEFAULT_PORT;
  tcp_queue_size = DEFAULT_QUEUE_SIZE;
  tcp_active = false;
  running = false;
}

void FileSyncServer::listen() {
  tcp.bind(tcp_port);
  tcp.listen(tcp_queue_size);
  tcp_active = true;
}

void FileSyncServer::run() {
  int errCode;
  pthread_t* thread_session = (pthread_t*) malloc(sizeof(pthread_t));
  FileSyncSession* session;
  for (;;) {
    try {
      session = accept();
      errCode = pthread_create(thread_session, NULL, run_session, (void*) session);
      if (errCode != 0) {
        session->close();
      }
    }
    catch (std::runtime_error e) {
      qlogmutex.lock();
      qlog.push(e.what());
      qlogmutex.unlock();
      continue;
    }
  }
}

FileSyncSession* FileSyncServer::accept() {
  FileSyncSession* session = new FileSyncSession;
  TCPConnection* conn;
  conn = tcp.accept();
  session->tcp = conn;
  session->active = true;
  session->user = NULL;
  return session;
}

void FileSyncServer::set_port(int port) {
  tcp_port = port;
}

void FileSyncServer::set_queue_size(int queue_size) {
  tcp_queue_size = queue_size;
}


FileSyncSession::FileSyncSession(void) {
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

void FileSyncSession::close() {
  tcp->close();
  active = false;
}
