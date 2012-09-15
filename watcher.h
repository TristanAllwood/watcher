#ifndef __WATCHER_H
#define __WATCHER_H

#include "config.h"

static const long  TIMEOUT_NS = 80 * 1e6;

typedef struct watched_stanza {
  int num_file_descriptors;
  int * file_descriptors;
  stanza_t * stanza;
} watched_stanza_t;

#endif
