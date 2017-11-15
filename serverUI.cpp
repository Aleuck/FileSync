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
  int sessions = server->countSessions();
  int users = server->countUsers();
  getmaxyx(stdscr, h, w);

  // create log window
  WINDOW* info_win = newwin(h/2, w/2, 0, 0);
  WINDOW* log_win = newwin(h, w/2, 0, w/2);
  nodelay(info_win, TRUE);
  nodelay(log_win, TRUE);
  box(info_win, 0, 0);
  box(log_win, 0, 0);
  // refresh();
  //start running
  wrefresh(info_win);
  wrefresh(log_win);
  while (running) {
    getmaxyx(stdscr, h, w);

    if (server->qlogsemaphore.trywait() == 0) {
      server->qlogmutex.lock();
      int lines = 0;
      werase(log_win);
      for (auto ii = server->qlog.begin(); ii != server->qlog.end() && lines < 10; ++ii) {
        wmove(log_win, h-3-(lines++), 2);
        wprintw(log_win, (*ii).c_str());
      }
      server->qlogmutex.unlock();
      box(log_win, 0, 0);
      wrefresh(log_win);
    }

    sessions = server->countSessions();
    users = server->countUsers();
    wmove(info_win, 1, 2);
    wprintw(info_win ,"sessions: %2d", sessions);
    wmove(info_win, 3, 2);
    wprintw(info_win ,"users: %2d", users);

    wmove(info_win, 0, 0);
    wrefresh(info_win);
    if ((ch = wgetch(info_win)) == ERR) {
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
