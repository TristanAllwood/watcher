#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "util.h"

static config_error_t parse_patterns(char ***, char *);
static config_error_t parse_commands(char ***, char *, FILE *);

enum { BUFFER_SIZE = 512,
       PATTERN_LIMIT = 5,
       COMMAND_LIMIT = 5,
       STANZA_LIMIT  = 5};

config_error_t parse_config(config_t **out, FILE *file) {
  
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
  (*out)->stanza_count = stanza_count;
  (*out)->stanzas = stanzas;

  return CONFIG_OK;
}

static config_error_t parse_patterns(char ***patterns, char *buffer) {
  char *save_ptr;
  *patterns = calloc(PATTERN_LIMIT, sizeof(char*));
  if (*patterns == NULL) {
    return CONFIG_ERRNO;
  }

  char **current_pattern = *patterns;

  do {
    if (current_pattern - *patterns >= PATTERN_LIMIT) {
      // TODO: free stuff
      return CONFIG_TOO_MANY_PATTERNS;
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
  } while (fgets(buffer, BUFFER_SIZE, file) != NULL && *buffer != '\0' && *buffer != '\n');

  return CONFIG_OK;
}

