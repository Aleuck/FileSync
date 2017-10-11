#include "util.hpp"
#include "server.hpp"
#include "serverUI.hpp"
#include "tcp.hh"
#include <mutex>

#define MAX_CON 2

using namespace std;

void* run_server(void* arg);
void close_server(int sig);

pthread_t thread_ui;
pthread_t thread_server;
queue<string> server_log;
mutex server_log_mutex;

int main(int argc, char* argv[]) {
  TCPServer* server;
  int errCode, port;
  signal(SIGINT, close_server);
  if (argc < 2) {
    cerr << "Usage: " << argv[0] << " <port>" << endl;
    exit(0);
  }
  port = atoi(argv[1]);
  cout << "starting service..." << endl;
  try {
    server = new TCPServer(port);
  }
  catch (runtime_error e) {
    cerr << e.what() << endl;
    exit(1);
  }
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
