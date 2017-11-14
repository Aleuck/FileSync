#include "server.hpp"
#include "util.hpp"
#include "serverUI.hpp"

void mainScreen();
void close_serverUI(int sig);

using namespace std;

FSServerUI::FSServerUI(FileSyncServer* serv) {
  server = serv;
}

void FSServerUI::start() {
  init();
  running = true;
  thread = std::thread(&FSServerUI::run, this);
}

void FSServerUI::close() {
  running = false;
  thread.join();
}

void FSServerUI::run() {
  int ch;
  int h, w;
  //WINDOW* win = newwin();
  while (running) {
    getmaxyx(stdscr, h, w);
    move(1,1);
    attron(A_STANDOUT);
    printw("sessions: %2d users: %d, colors: %3d ", server->countSessions(), server->countUsers(), COLOR_PAIRS);
    server->qlogmutex.lock();
    int lines = 0;
    for (auto ii = server->qlog.begin(); ii != server->qlog.end() && lines < 10; ++ii) {
      move(12-(lines++),1);
      printw((*ii).c_str());
    }
    server->qlogmutex.unlock();
    refresh();
    if ((ch = getch()) == ERR) {
      // no input
    } else {
      // user pressed a key
      switch (ch) {
        case 'q':
        case 'Q':
          server->stop();
          break;
        default:
          break;
      }
    }
  }
  end();
}
