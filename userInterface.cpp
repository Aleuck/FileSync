#include "userInterface.hpp"
#include <iostream>

using namespace std;


void FilesyncUI::init() {
  // cerr <<  << " color pairs\n";
  initscr(); // initialize ncurses
  cbreak(); // disable input buffering
  noecho(); // dont echo typed chars
  keypad(stdscr, TRUE); // capture special chars
  nodelay(stdscr, TRUE);
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_WHITE,   COLOR_BLACK);
    init_pair(2, COLOR_WHITE,    COLOR_BLUE);
    init_pair(3, COLOR_GREEN,   COLOR_BLACK);
    init_pair(4, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(5, COLOR_RED,     COLOR_BLACK);
    init_pair(6, COLOR_CYAN,    COLOR_BLACK);
    init_pair(7, COLOR_MAGENTA, COLOR_BLACK);
  }
}
void FilesyncUI::end() {
  endwin(); // end ncurses
}

void FilesyncUI::getScreenSize(int &h, int &w) {
  getmaxyx(stdscr, h, w);
}
