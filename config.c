#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "util.h"

static config_error_t parse_stanza(stanza_t *, char *, char *, FILE *file);
static config_error_t parse_patterns(char ***, char *);
static config_error_t parse_commands(char ***, char *, FILE *);

enum { BUFFER_SIZE = 512,
       PATTERN_LIMIT = 5,
       COMMAND_LIMIT = 5,
       STANZA_LIMIT  = 5};

config_error_t parse_config(config_t **out, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    return CONFIG_ERRNO;
  }

  char buffer[BUFFER_SIZE];

  stanza_t *stanzas = calloc(STANZA_LIMIT, sizeof(stanza_t));
  if (stanzas == NULL) {
    return CONFIG_ERRNO;
  }

  stanza_t *current_stanza = stanzas;

  while (fgets(buffer, BUFFER_SIZE, file) != NULL) {

    if (current_stanza - stanzas >= STANZA_LIMIT) {
      // TODO: free stanzas, patterns, commands, etc.
      return CONFIG_TOO_MANY_STANZAS;
    }

    // ignore comments
    char *hash = strchr(buffer, '#');
    if (hash != NULL) {
      *hash = '\0';
    }

    char *colon = strrchr(buffer, ':');
    // ignore lines without a colon
    if (colon == NULL) {
      continue;
    }

    *colon = '\0';

    config_error_t error;
    error = parse_stanza(current_stanza, buffer, colon, file);

    if (error != CONFIG_OK) {
      // TODO free stuff?
      return error;
    }
    current_stanza++;

  }

  int stanza_count = current_stanza - stanzas;
  current_stanza = NULL;
  stanzas = realloc(stanzas, stanza_count * sizeof(stanza_t));
  if (stanzas == NULL) {
    // TOOD: free stuff
    return CONFIG_ERRNO;
  }

  *out = malloc(sizeof(config_t));
  if (*out == NULL) {
    // TODO: free stuff
    return CONFIG_ERRNO;
  }

  if (fclose(file)) {
    // TODO: free things
    return CONFIG_ERRNO;
  }

  (*out)->stanza_count  = stanza_count;
  (*out)->stanzas       = stanzas;
  (*out)->watcher_file  = strdup(filename);

  return CONFIG_OK;
}

static config_error_t parse_stanza(stanza_t *current_stanza,
                                    char *buffer, char *colon, FILE *file) {
  char **patterns;
  char **commands;
  config_error_t error;

  error = parse_patterns(&patterns, buffer);
  if (error != CONFIG_OK) {
    // TODO: free stanzas, patterns, etc
    return error;
  }

  error = parse_commands(&commands, colon + 1, file);
  if (error != CONFIG_OK) {
    // TODO: free stanzas, patterns, commands, etc.
    return error;
  }

  current_stanza->patterns = patterns;
  current_stanza->commands = commands;

  return CONFIG_OK;
}

static config_error_t parse_patterns(char ***patterns, char *buffer) {
  char *save_ptr;
  int allocated_pattern_count = PATTERN_LIMIT;
  *patterns = calloc(allocated_pattern_count, sizeof(char *));
  if (*patterns == NULL) {
    return CONFIG_ERRNO;
  }

  char **current_pattern = *patterns;

  do {
    if (current_pattern - *patterns >= allocated_pattern_count) {
      int current_pattern_offset = current_pattern - *patterns;
      allocated_pattern_count += PATTERN_LIMIT;
      errno = 0;
      *patterns = realloc(*patterns, allocated_pattern_count * sizeof(char *));
      if (errno != 0) {
        // TODO: free stuff
        return CONFIG_ERRNO;
      }
      current_pattern = (*patterns)+current_pattern_offset;
    }

    char *tmp = strtok_r(buffer, " \t", &save_ptr);
    *current_pattern = tmp != NULL ? strdup(tmp) : NULL;
    buffer = NULL;
  } while (*(current_pattern++) != NULL);

  // TODO: realloc patterns.

  return CONFIG_OK;
}

static config_error_t parse_commands(char ***out, char *buffer, FILE *file) {

  *out = calloc(COMMAND_LIMIT, sizeof(char *));
  if (*out == NULL) {
    return CONFIG_ERRNO;
  }

  char **current_command = *out;

  do {
    if (current_command - *out >= COMMAND_LIMIT) {
      //TODO: free stuff
      return CONFIG_TOO_MANY_COMMANDS;
    }

    size_t skip = strspn(buffer, " \t");
    char *command = buffer + skip;

    chomp(command);
    if (command[0] == '\0') {
      continue;
    }
    *current_command = strdup(command);
    if (*current_command == NULL) {
      // TODO: free stuff
      return CONFIG_ERRNO;
    }

    current_command++;
  } while (fgets(buffer, BUFFER_SIZE, file) != NULL &&
                         *buffer != '\0' && *buffer != '\n');

  return CONFIG_OK;
}

void free_config(config_t **config) {

  for(stanza_t *current_stanza = (*config)->stanzas ;
      current_stanza - (*config)->stanzas < (*config)->stanza_count ;
      current_stanza++) {

    for(char **pattern = current_stanza->patterns;
        *pattern != NULL;
        pattern++ ) {
      free(*pattern);
    }

    for(char **command = current_stanza->commands;
        *command != NULL;
        command++ ) {
      free(*command);
    }

    free(current_stanza->patterns);
    free(current_stanza->commands);

  }

  free ((*config)->stanzas);
  free ((*config)->watcher_file);

  free(*config);
  *config = NULL;
}

