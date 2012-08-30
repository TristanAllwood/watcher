#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "util.h"

static config_error_t parse_stanza(stanza_t *, char *, char *, FILE *file);
static config_error_t parse_commands(char ***, char *, FILE *);

enum { BUFFER_SIZE = 512,
       COMMAND_INCREMENT = 5,
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
  char **commands;
  config_error_t error;

  error = parse_commands(&commands, colon + 1, file);
  if (error != CONFIG_OK) {
    // TODO: free stanzas, patterns, commands, etc.
    return error;
  }

  current_stanza->pattern = strdup(buffer);
  if (current_stanza->pattern == NULL) {
    return CONFIG_ERRNO;
  }
  current_stanza->commands = commands;

  return CONFIG_OK;
}

static config_error_t parse_commands(char ***out, char *buffer, FILE *file) {

  *out = calloc(COMMAND_INCREMENT, sizeof(char *));
  int max_commands = COMMAND_INCREMENT;
  if (*out == NULL) {
    return CONFIG_ERRNO;
  }

  char **current_command = *out;

  do {
    if (current_command - *out == max_commands - 1) {
      *out = realloc(*out, sizeof(char *) * (max_commands + COMMAND_INCREMENT));
      current_command = (*out) + max_commands;
      max_commands += COMMAND_INCREMENT;
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
    *current_command = NULL;
  } while (fgets(buffer, BUFFER_SIZE, file) != NULL &&
                         *buffer != '\0' && *buffer != '\n');

  return CONFIG_OK;
}

void free_config(config_t **config) {

  for(stanza_t *current_stanza = (*config)->stanzas ;
      current_stanza - (*config)->stanzas < (*config)->stanza_count ;
      current_stanza++) {

    for(char **command = current_stanza->commands;
        *command != NULL;
        command++ ) {
      free(*command);
    }

    free(current_stanza->pattern);
    free(current_stanza->commands);

  }

  free ((*config)->stanzas);
  free ((*config)->watcher_file);

  free(*config);
  *config = NULL;
}

const char * str_config_error(config_error_t error) {
  switch(error) {
    case CONFIG_OK:
      return "OK";
    case CONFIG_PARSE_ERROR:
      return "PARSE ERROR";
    case CONFIG_TOO_MANY_STANZAS:
      return "TOO MANY STANZAS";
    case CONFIG_ERRNO:
      return strerror(errno);
    default:
      return "IMPOSSIBLE";
  }
}
