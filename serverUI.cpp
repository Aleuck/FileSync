#include <iostream>
#include <cstdlib>
#include <ncurses.h>
#include "userInterface.h"
#include "server.h"

void mainScreen();

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
      }
    }
  }
}
