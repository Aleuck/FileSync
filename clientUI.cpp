#include "util.hpp"
#include "client.hpp"
#include "clientUI.hpp"

void mainScreen();

using namespace std;

FSClientUI::FSClientUI(FileSyncClient *clnt) {
  client = clnt;
}

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
  int h;
  int w;
  getmaxyx(stdscr, h, w);
  WINDOW* log_win = newwin(h, w/2, 0, w/2);
  nodelay(log_win, TRUE);
  box(log_win, 0, 0);
  wrefresh(log_win);
  while (running) {
    getmaxyx(stdscr, h, w);
    if (client->qlog_sem.trywait() == 0) {
      client->qlog_mutex.lock();
      int lines = 0;
      werase(log_win);
      for (auto ii = client->qlog.begin(); ii != client->qlog.end() && lines < (h-5); ++ii) {
        wmove(log_win, h-3-(lines++), 2);
        wprintw(log_win, (*ii).c_str());
      }
      client->qlog_mutex.unlock();
      box(log_win, 0, 0);
    }
    wmove(log_win,0,0);
    wrefresh(log_win);
    if ((ch = wgetch(log_win)) == ERR) {
      // no input
    } else {
      // user pressed a key
      switch (ch) {
        case 'q':
        case 'Q':
          client->stop();
          break;
        default:
          break;
      }
    }
  }
  end();
}
