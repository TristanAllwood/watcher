#define _XOPEN_SOURCE

#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wordexp.h>
#include <stdbool.h>

#include "config.h"
#include "util.h"
#include "watcher.h"

static void main_loop(config_t *);
static void create_watched_stanza(watched_stanza_t *, stanza_t *, int);

int main(int argc, char ** argv) {
  config_t *config;
  config_error_t error;

  FILE *watcher = fopen(".watcher", "r");

  error = parse_config(&config, watcher);

  if (error == CONFIG_OK) {
    main_loop(config);
  }
}

static void main_loop(config_t *config) {

  int inotify_fd = inotify_init();
  if (inotify_fd == -1) {
    perror("inotify_init");
    exit(1);
  }

  while(true) { // TODO: exit and cleanup from loop nicely

    watched_stanza_t *watched_stanzas
      = calloc(config->stanza_count, sizeof(watched_stanza_t));
    if (watched_stanzas == NULL) {
      perror("calloc");
      exit(1);
    }

    watched_stanza_t *current_watched_stanza = watched_stanzas;
    for (stanza_t *current_stanza = config->stanzas;
         current_stanza - config->stanzas < config->stanza_count;
         current_stanza++, current_watched_stanza++) {

      create_watched_stanza(current_watched_stanza, current_stanza, inotify_fd);
    }

    struct inotify_event event;
    ssize_t r = read(inotify_fd, &event, sizeof(struct inotify_event));
    if (r != sizeof(struct inotify_event)) {
      fprintf(stderr, "inotify read\n");
      exit(1);
    }

    for (current_watched_stanza = watched_stanzas;
         current_watched_stanza - watched_stanzas < config->stanza_count;
         current_watched_stanza++) {

      if (bsearch_i(current_watched_stanza->num_file_descriptors,
                      event.wd, current_watched_stanza->file_descriptors)) {

        for(char **curr_command = current_watched_stanza->stanza->commands;
            *curr_command != NULL;
            curr_command++) {
          printf("Running: %s\n", *curr_command);
          system("sync"); // TODO: replace with fsync or something on the
                          // file that has changed to stop races
          int status = system(*curr_command);
          if (status != 0) {
            printf("command exited with code: %d\n", status);
          }
        }
      }

    }
  }

}

static void create_watched_stanza(watched_stanza_t *current_watched_stanza,
                        stanza_t *current_stanza, int inotify_fd) {
  int error;
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


  current_watched_stanza->num_file_descriptors = paths.we_wordc;
  current_watched_stanza->file_descriptors = calloc(paths.we_wordc,
                                                      sizeof(int *));
  if (current_watched_stanza->file_descriptors == NULL) {
    perror("calloc");
    exit(1);
  }
  current_watched_stanza->stanza = current_stanza;

  for (int i = 0; i < paths.we_wordc ; i++) {
    int path_descriptor = inotify_add_watch(inotify_fd, paths.we_wordv[i],
        IN_MODIFY | IN_MOVE_SELF | IN_ATTRIB);
    if (path_descriptor == -1) {
      perror("inotify_add_watch");
      exit(1);
    }
    current_watched_stanza->file_descriptors[i] = path_descriptor;
  }

  qsort_i(paths.we_wordc, current_watched_stanza->file_descriptors);

  wordfree(&paths);
}
