#ifndef __WATCHER_H
#define __WATCHER_H

#include "config.h"

typedef struct watched_stanza {
  int num_file_descriptors;
  int * file_descriptors;
  stanza_t * stanza;
} watched_stanza_t;

#endif
