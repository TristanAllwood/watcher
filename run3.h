#ifndef __RUN_THREE_H
#define __RUN_THREE_H

struct run3result {
  char *stdout;
  char *stderr;
  int status;
};


struct run3result *run3(char *, char *);
#endif
