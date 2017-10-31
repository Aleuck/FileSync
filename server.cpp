#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"
#include "tcp.hh"
#include <mutex>

#define DEFAULT_QUEUE_SIZE 5
#define DEFAULT_MAX_CON 2
#define DEFAULT_PORT 53232

using namespace std;

void* run_server(void* arg);
void close_server(int sig);

pthread_t thread_ui;
pthread_t thread_server;

FSServer* server;

FSServer::FSServer(void) {
  port = DEFAULT_PORT;
  max_con = DEFAULT_MAX_CON;
  is_running = false;
}

int main(int argc, char* argv[]) {
  int errCode, port;
  signal(SIGINT, close_server);
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <port>" << endl;
    exit(0);
  }
  port = atoi(argv[1]);
  cout << "starting service..." << endl;
  server = new FSServer();
  errCode = pthread_create(&thread_server, NULL, run_server, (void*) server);
  cout << "starting interface..." << endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, (void*) server);
  if (errCode != 0) {
    cout << "Error starting interface." << endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  pthread_join(thread_server, NULL);
  delete server;
  exit(0);
}

void* run_server(void* arg) {
  TCPServer* serv = (TCPServer*) arg;
  TCPConnection* con;
  for (;;) {
      try {
        con = serv->accept();
        errCode = pthread_create(&thread_server, NULL, run_server, (void*) server);
      }
      catch (runtime_error e) {
          // put error in log
      }
  }
  return NULL;
}

void close_server(int sig) {
  pthread_kill(thread_ui, sig);
  pthread_join(thread_ui, NULL);
  cout << "Stopping service..." << sig << endl;
  pthread_kill(thread_server, sig);
  pthread_join(thread_server, NULL);
  exit(0);
}
