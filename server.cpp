#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include "serverUI.h"

using namespace std;

int main(int argc, char* argv[]) {
  pthread_t thread_ui;
  int errCode;
  cout << "starting interface..." << endl;
  errCode = pthread_create(&thread_ui, NULL, serverUI, NULL);
  if (errCode != 0) {
    cout << "error starting interface" << endl;
    exit(1);
  }
  pthread_join(thread_ui, NULL);
  exit(0);
}
