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
  if (cmd == "list") {
    return;
  }
  std::vector<std::string> cmds;
  boost::split(cmds, cmd, boost::is_any_of(" "));
  if (cmds.size() > 0) {
    if (cmds[0] == "upload") {
      if (cmds.size() != 2) {
        // output("`Wrong argument, correct usage: upload <filepath>");
        return;
      }
      client->enqueue_action(FilesyncAction(REQUEST_UPLOAD, cmds[1]));
      return;
    }
    if (cmds[0] == "download") {
      if (cmds.size() != 2) {
        // output("`Wrong argument, correct usage: download <filename>");
        return;
      }
      client->enqueue_action(FilesyncAction(REQUEST_DOWNLOAD, cmds[1]));
      return;
    }
    if (cmds[0] == "delete") {
      if (cmds.size() != 2) {
        // output("`Wrong argument, correct usage: download <filename>");
        return;
      }
      client->enqueue_action(FilesyncAction(REQUEST_DELETE, cmds[1]));
      return;
    }
    // output("`" + cmds[0] + "`: Invalid command.");
  }
}

void FSClientUI::run() {
  int ch, h, w;
  char input_buf[MAXNAME];
  int input_cur = 0;
  memset(input_buf, 0, MAXNAME);
  getmaxyx(stdscr, h, w);
  log_win = newwin(h-3, w/2, 0, w/2);
  cmd_win = newwin(3, w, h-3, 0);
  files_win = newwin(h-3, w/4, 0, 0);
  queue_win = newwin(h-3, w/4, 0, w/4);
  char decimalByteUnit[] = "B \0kB\0MB\0GB\0TB\0PB\0EB\0ZB\0YB";
  keypad(cmd_win, TRUE); // capture special chars
  nodelay(cmd_win, TRUE);
  boxTitle(files_win, "Files");
  boxTitle(log_win, "Log");
  boxTitle(queue_win, "Queue");
  box(cmd_win, 0, 0);
  mvwaddch(cmd_win, 1, 1, '>');
  wmove(cmd_win, 1, 3 + input_cur);
  wrefresh(log_win);
  wrefresh(files_win);
  wrefresh(queue_win);
  wrefresh(cmd_win);
  while (running) {
    getmaxyx(stdscr, h, w);
    if (client->qlog_sem.trywait() == 0) {
      client->qlog_mutex.lock();
      int lines = 0;
      werase(log_win);
      boxTitle(log_win, "Log");
      for (auto ii = client->qlog.begin(); ii != client->qlog.end() && lines < (h-4); ++ii) {
        wmove(log_win, h-5-(lines++), 2);
        wprintw(log_win, (*ii).c_str());
      }
      client->qlog_mutex.unlock();
      wrefresh(log_win);
      wrefresh(cmd_win);
    }
    if (client->files_sem.trywait() == 0) {
      client->files_mtx.lock();
      int lines = 0;
      char info[w/4];
      werase(files_win);
      boxTitle(files_win, "Files");
      mvwprintw(files_win, 1, 2, "Name");
      // " Size     Last Modified      "
      // "100.0 GB  2017-12-28T16:01:40"
      mvwprintw(files_win, 1, w/4-30, " Size     Last Modified");
      for (auto ii = client->files.begin(); ii != client->files.end() && lines < (h-4); ++ii) {
        mvwprintw(files_win, 2+lines, 2, ii->first.c_str());
        double size = ii->second.size;
        uint32_t unitprefix = 0;
        while (size >= 1000) {
          size /= 1000;
          unitprefix++;
        }
        string mtime = flocaltime("%F %T", ii->second.last_mod);
        sprintf(info, "%5.1f %s  %s", size, decimalByteUnit + 3 * unitprefix, mtime.c_str());
        mvwprintw(files_win, 2 + lines, w/4-30, info);
        lines += 1;
        //
      }
      client->files_mtx.unlock();
      wrefresh(files_win);
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
