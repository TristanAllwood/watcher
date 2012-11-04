#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _POSIX_C_SOURCE 199309L

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <time.h>
#include <unistd.h>
#include <wordexp.h>

#include "config.h"
#include "util.h"
#include "watcher.h"

static void create_watched_stanza(watched_stanza_t *, stanza_t *, int);
static void setup_watched_stanzas(config_t *, int, watched_stanza_t **);
static void free_watched_stanzas(config_t *, watched_stanza_t **);

int main(int argc, char ** argv) {

  const char *config_file = ".watcher";
  config_t *config;
  config_error_t error;

  error = parse_config(&config, config_file);

  if (error != CONFIG_OK) {
    const char * errmsg = str_config_error(error);
    fprintf(stderr, "Error: %s\n", errmsg);
    exit(error);
  }

  while(true) {

    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
      perror("inotify_init");
      exit(1);
    }

    watched_stanza_t *watched_stanzas;
    setup_watched_stanzas(config, inotify_fd, &watched_stanzas);
    int config_file_descriptor;

    config_file_descriptor = inotify_add_watch(inotify_fd, config_file,
                                          IN_MOVE_SELF | IN_MODIFY | IN_ATTRIB);
    if (config_file_descriptor == -1) {
      perror("inotify_add_watch");
      exit(1);
    }

    struct inotify_event event;
    ssize_t r = read(inotify_fd, &event, sizeof(struct inotify_event));
    if (r != sizeof(struct inotify_event)) {
      fprintf(stderr, "inotify read\n");
      exit(1);
    }

    /* need to drain off other events, allow TIMEOUT between each one */
    fcntl(inotify_fd, F_SETFL, O_NONBLOCK);

    struct inotify_event tmp_event;
    bool event_happend;

    do {
      event_happend = false;
      struct timespec ts;
      ts.tv_sec = 0;
      ts.tv_nsec = TIMEOUT_NS;

      if (nanosleep(&ts, NULL) == -1) {
        fprintf(stderr, "nanosleep\n");
        exit(1);
      }

      while (read(inotify_fd, &tmp_event, sizeof(struct inotify_event) > 0)) {
        event_happend = true;
      }
    } while (event_happend);


    /* now process the original event */
    watched_stanza_t *current_watched_stanza = watched_stanzas;

    for (current_watched_stanza = watched_stanzas;
         current_watched_stanza - watched_stanzas < config->stanza_count;
         current_watched_stanza++) {

      if (bsearch_i(current_watched_stanza->num_file_descriptors,
                      event.wd, current_watched_stanza->file_descriptors)) {

        for(char **curr_command = current_watched_stanza->stanza->commands;
            *curr_command != NULL;
            curr_command++) {
          int status = system(*curr_command);
          if (status != 0) {
            printf("command exited with code: %d\n", status);
          }
        }
      }

    }

    free_watched_stanzas(config, &watched_stanzas);

    if (event.wd == config_file_descriptor) {
      free_config(&config);

      printf("watcher: Reloading config file:\n");
      error = parse_config(&config, config_file);

      if (error != CONFIG_OK) {
        if (error == CONFIG_ERRNO) {
          perror("parse config");
        }
        exit(error);
      }
    }

    if(close(inotify_fd) != 0) {
      perror("close inotify_fd");
      exit(1);
    }

  }
}

static void setup_watched_stanzas(config_t *config,
                          int inotify_fd, watched_stanza_t **watched_stanzas) {

    *watched_stanzas = calloc(config->stanza_count, sizeof(watched_stanza_t));
    if (*watched_stanzas == NULL) {
      perror("calloc");
      exit(1);
    }

    watched_stanza_t *current_watched_stanza = *watched_stanzas;
    for (stanza_t *current_stanza = config->stanzas;
         current_stanza - config->stanzas < config->stanza_count;
         current_stanza++, current_watched_stanza++) {
      create_watched_stanza(current_watched_stanza, current_stanza, inotify_fd);
    }
}

static void create_watched_stanza(watched_stanza_t *current_watched_stanza,
                        stanza_t *current_stanza, int inotify_fd) {
  int error;
  wordexp_t paths;

  char *pattern = current_stanza->pattern;
  if ((error = wordexp(pattern, &paths, WRDE_SHOWERR | WRDE_UNDEF)) != 0) {
    fprintf(stderr, "wordexp error: %d\n", error);
    exit(1);
  }

  current_watched_stanza->num_file_descriptors = paths.we_wordc;
  current_watched_stanza->file_descriptors = calloc(paths.we_wordc,
                                                      sizeof(int));
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

static void free_watched_stanzas(config_t *config,
                                 watched_stanza_t **watched_stanzas) {
    for (watched_stanza_t *current_watched_stanza = *watched_stanzas;
         current_watched_stanza - *watched_stanzas < config->stanza_count;
         current_watched_stanza++) {
      free(current_watched_stanza->file_descriptors);
    }

    free(*watched_stanzas);
    *watched_stanzas = NULL;
}
