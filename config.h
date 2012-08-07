#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>

typedef struct stanza {
  char **patterns;
  char **commands;
} stanza_t;

typedef struct config {
  int stanza_count;
  stanza_t *stanzas;
  char *watcher_file;
} config_t;

typedef enum config_error {
  CONFIG_OK,
  CONFIG_PARSE_ERROR,
  CONFIG_TOO_MANY_PATTERNS,
  CONFIG_TOO_MANY_STANZAS,
  CONFIG_TOO_MANY_COMMANDS,
  CONFIG_ERRNO
} config_error_t;


config_error_t parse_config(config_t **, const char *);
void free_config(config_t **);

#endif
