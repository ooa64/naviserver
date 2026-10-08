#ifndef strndup

#include <string.h>
#include "../include/nsthread.h"

char *strndup(const char *s, size_t n) {
  size_t len = strnlen(s, n);
  char *p = ns_malloc(len + 1);
  if (p) {
    memcpy(p, s, len);
    p[len] = 0;
  }
  return p;
}

#endif