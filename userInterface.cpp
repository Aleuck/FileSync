#include <iostream>
#include <cstdlib>
#include <ncurses.h>

using namespace std;

void startUI() {
  std::cout << "Hello" << std::endl;
  initscr(); // initialize ncurses
  cbreak(); // disable input buffering
  noecho(); // dont echo typed chars
  keypad(stdscr, TRUE); // capture special chars
  nodelay(stdscr, TRUE);
  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED,     COLOR_BLACK);
    init_pair(2, COLOR_GREEN,   COLOR_BLACK);
    init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
    init_pair(4, COLOR_BLUE,    COLOR_BLACK);
    init_pair(5, COLOR_CYAN,    COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE,   COLOR_BLACK);
  }
}

void endUI() {
  endwin(); // end ncurses
}
