#ifndef HEADER_UI
#define HEADER_UI

#include <ncurses.h>
#include <string>

class FilesyncUI;

class FilesyncUI{
protected:
  void init();
  void end();
  void getScreenSize(int &h, int &w);
  void boxTitle(WINDOW *win, std::string title);
};

#endif
