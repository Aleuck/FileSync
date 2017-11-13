#include "util.hpp"
#include "client.hpp"
#include "clientUI.hpp"

void mainScreen();

using namespace std;

void FSClientUI::start() {
  init();
  running = true;
  thread = std::thread(&FSClientUI::run, this);
}

void FSClientUI::close() {
  running = false;
  thread.join();
}

void FSClientUI::run() {
  int ch;
  while (running) {
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
  end();
}
