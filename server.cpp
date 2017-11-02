#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"
#include "tcp.hh"
#include <mutex>

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_PORT 53232
#define DEFAULT_MAX_CON 2

void* run_server(void* arg);
void* handle_session(void* arg);
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
  FileSyncSession* session;
  serv->run();
  return NULL;
}
void* handle_session(void* arg) {
  FileSyncSession* session = (FileSyncSession*) arg;
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
  FileSyncSession* session;
  for (;;) {
    try {
      session = accept();
      errCode = pthread_create(&thread_server, NULL, handle_session, (void*) session);
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

void FileSyncSession::close() {
  tcp->close();
}
