#include <iostream>
#include <csignal>
#include <cstdlib>
#include <ncurses.h>

#include "userInterface.hpp"
#include "clientUi.hpp"
#include "client.hpp"

void mainScreen();

using namespace std;

void* serverUI(void* arg) {
  startUI();
  mainScreen();
  endUI();
  return NULL;
}

void mainScreen() {
  int ch;
  for (;;) {
    if ((ch = getch()) == ERR) {
      // no input
    } else {
      // user pressed a key
      switch (ch) {
        case 'q':
        case 'Q':
          return;
        default:
          break;
      }
    }
  }
}
