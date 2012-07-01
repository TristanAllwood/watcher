#define _XOPEN_SOURCE

#include <stdlib.h>
#include <wordexp.h>
#include <sys/inotify.h>

#include "config.h"

static void main_loop(config_t *);

int main(int argc, char ** argv) {
  config_t *config;
  config_error_t error;

  FILE *watcher = fopen(".watcher", "r");

  error = parse_config(&config, watcher);

  printf("Error: %d, Stanzas: %d\n", error, config->stanza_count);

  if (error == CONFIG_OK) {
    main_loop(config);
  }
}

static void main_loop(config_t *config) {
  int error;

  int inotify_fd = inotify_init();
  if (inotify_fd == -1) {
    perror("inotify_init");
    exit(1);
  }
  
  for (stanza_t *current_stanza = config->stanzas; 
       current_stanza - config->stanzas < config->stanza_count;
       current_stanza++) {

    wordexp_t paths; 
    int wordexp_flags = 0;

    for (char **pattern = current_stanza->patterns; 
         *pattern != NULL; 
         pattern++) {
      if ((error = wordexp(*pattern, &paths, wordexp_flags)) != 0) {
        fprintf(stderr, "wordexp error: %d\n", error);
        exit(1);
      }
      wordexp_flags |= WRDE_APPEND;
    }


    for (int i = 0; i < paths->we_wordc ; i++) {
      int path_descriptor = inotify_add_watch(inotify_fd, paths->we_wordv[i], 
          IN_MODIFY | IN_MOVE_SELF);
      if (path_descriptor == -1) {
        perror("inotify_add_watch");
        exit(1);
      }
    }

    wordfree(paths);

  }
}
