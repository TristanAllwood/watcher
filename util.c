#include <string.h>
#include <ctype.h>

#include "util.h"

void chomp(char * s) {
  int len = strlen(s);

  for(int i = len - 1; i >= 0 && (isspace(s[i])) ; i--) {
    s[i] = '\0';
  }
}
