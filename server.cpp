#include <iostream>
#include <csignal>
#include <cstdlib>
#include <string>
#include <vector>
#include <pthread.h>

#include "serverUI.hpp"


void finish(int sig);

pthread_t thread_ui;

using namespace std;

int main(int argc, char* argv[]) {
  int errCode;
  (void) signal(SIGINT, finish);
  cout << "starting interface..." << endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, NULL);
  if (errCode != 0) {
    cout << "error starting interface" << endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  exit(0);
}

void finish(int sig) {
  cout << "server signal " << sig << endl;
}
