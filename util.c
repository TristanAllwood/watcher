#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

static int int_cmp(const void *, const void *);

void chomp(char * s) {
  int len = strlen(s);

  for(int i = len - 1; i >= 0 && (isspace(s[i])) ; i--) {
    s[i] = '\0';
  }
}

void qsort_i(size_t length, int array[]) {
  qsort(array, length, sizeof(int), int_cmp);
}

int bsearch_i(size_t length, int key, int array[]) {
  return bsearch(&key, array, length, sizeof(int), int_cmp) != NULL;
}

static int int_cmp(const void * left, const void * right) {
  int l = * (int*) left;
  int r = * (int*) right;
  return (l > r) - (r > l);
}


