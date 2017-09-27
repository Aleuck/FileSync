#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>
#include <pthread.h>

#include "serverUI.hpp"


void* server(void* arg);
void close_server(int sig);

pthread_t thread_ui;
pthread_t thread_server;

using namespace std;

int main(int argc, char* argv[]) {
  int errCode;
  signal(SIGINT, close_server);
  cout << "starting service...";
  errCode = pthread_create(&thread_server, NULL, server, NULL);
  cout << "starting server interface..." << endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, NULL);
  if (errCode != 0) {
    cout << "error starting interface" << endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  pthread_join(thread_server, NULL);
  exit(0);
}

void* server(void* arg) {
  return NULL;
}

void close_server(int sig) {
  kill(thread_ui, sig);
  pthread_join(thread_ui, NULL);
  cout << "Stopping service..." << sig << endl;
  kill(thread_server, sig);
  pthread_join(thread_server, NULL);
  exit(0);
}
