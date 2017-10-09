#include "util.hpp"
#include "userInterface.hpp"
#include "serverUI.hpp"
#include "server.hpp"

void mainScreen();
void close_serverUI(int sig);

using namespace std;

void* serverUI(void* arg) {
  signal(SIGINT, close_serverUI);
  startUI();
  mainScreen();
  endUI();
  return NULL;
}

void mainScreen() {
  int ch;
  int h, w;
  //WINDOW* win = newwin();
  for (;;) {
    getmaxyx(stdscr, h, w);
    refresh();
    move(h-1,w-25);
    attron(A_STANDOUT);
    printw("ch: %d h: %3d, w: %3d", ch, h, w);
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

void close_serverUI(int sig){
  endUI();
  exit(0);
}
