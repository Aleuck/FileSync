#ifndef HEADER_UI
#define HEADER_UI

#include <ncurses.h>


class FilesyncUI;

class FilesyncUI{
protected:
  void init();
  void end();
  void getScreenSize(int &h, int &w);
};

#endif
