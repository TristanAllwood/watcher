#include <stdlib.h>

#include "config.h"

int main(int argc, char ** argv) {
  config_t *config;
  config_error_t error;

  FILE *watcher = fopen(".watcher", "r");

  error = parse_config(&config, watcher);

  printf("Error: %d, Stanzas: %d", error, config->stanza_count);
}
