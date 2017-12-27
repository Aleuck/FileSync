#include "util.hpp"
#include "client.hpp"
#include "clientUI.hpp"
#include <boost/algorithm/string.hpp>

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
  if (running) {
    running = false;
    thread.join();
  }
}

void FSClientUI::exec_cmd(std::string cmd) {
  if (cmd == "quit") {
    client->enqueue_action(FilesyncAction(REQUEST_LOGOUT, ""));
    return;
  }
  std::vector<std::string> cmds;
  boost::split(cmds, cmd, boost::is_any_of(" "));
  if (cmds.size() > 0) {
    if (cmds[0] == "ls") {
      //
    } else if (cmds[0] == "upload") {
      //
    } else if (cmds[0] == "download") {
      //
    } else {
      output("`" + cmds[0] + "`: Invalid command.")
    }
  }
}

void FSClientUI::run() {
  int ch;
  int h;
  int w;
  char input_buf[MAXNAME];
  int input_cur = 0;
  memset(input_buf, 0, MAXNAME);
  getmaxyx(stdscr, h, w);
  WINDOW* log_win = newwin(h-3, w/2, 0, w/2);
  WINDOW* cmd_win = newwin(3, w, h-3, 0);
  nodelay(log_win, TRUE);
  box(log_win, 0, 0);
  wrefresh(log_win);
  keypad(cmd_win, TRUE); // capture special chars
  nodelay(cmd_win, TRUE);
  box(cmd_win, 0, 0);
  mvwaddch(cmd_win, 1, 1, '>');
  wmove(cmd_win, 1, 3 + input_cur);
  wrefresh(cmd_win);
  while (running) {
    getmaxyx(stdscr, h, w);
    if (client->qlog_sem.trywait() == 0) {
      client->qlog_mutex.lock();
      int lines = 0;
      werase(log_win);
      box(log_win, 0, 0);
      for (auto ii = client->qlog.begin(); ii != client->qlog.end() && lines < (h-4); ++ii) {
        wmove(log_win, h-5-(lines++), 2);
        wprintw(log_win, (*ii).c_str());
      }
      client->qlog_mutex.unlock();
      wrefresh(log_win);
      wrefresh(cmd_win);
    }
    if ((ch = wgetch(cmd_win)) == ERR) {
      // no input
    } else {
      // user pressed a key
      if (ch >= ' ' && ch <= '~') {
        if (input_cur < MAXNAME && input_cur < w - 6) {
          mvwaddch(cmd_win, 1, 3 + input_cur, ch);
          input_buf[input_cur++] = ch;
          wmove(cmd_win, 1, 3 + input_cur);
          wrefresh(cmd_win);
        }
      } else switch (ch) {
        case KEY_ENTER:
        case 10: {
          string input = input_buf;
          boost::trim(input);
          exec_cmd(input);
          // empty the buffer
          memset(input_buf, 0, MAXNAME);
          // clear input UI
          while (input_cur > 0) {
            input_cur -= 1;
            mvwaddch(cmd_win, 1, 3 + input_cur, ' ');
            wmove(cmd_win, 1, 3 + input_cur);
          }
          wrefresh(cmd_win);
        } break;
        case KEY_BACKSPACE: {
          if (input_cur > 0) {
            input_buf[--input_cur] = 0;
            mvwaddch(cmd_win, 1, 3 + input_cur, ' ');
            wmove(cmd_win, 1, 3 + input_cur);
            wrefresh(cmd_win);
          }
        } break;
        default:
          break;
      }
    }
  }
  end();
}
