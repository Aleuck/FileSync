#include <iostream>
#include <cstdlib>
#include <ncurses.h>

void startUI() {
  std::cout << "Hello" << std::endl;
  initscr(); // initialize ncurses
  cbreak(); // disable input buffering
  noecho(); // dont echo typed chars
  keypad(stdscr, TRUE); // capture special chars
  nodelay(stdscr, TRUE);
}

void endUI() {
  endwin(); // end ncurses
}
