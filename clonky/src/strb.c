#include "strb.h"
#include <stdio.h>

inline void strb_init(strb *self) {
  self->index = 0;
  self->chars[0] = '\0';
}

inline size_t strb_rem(strb *self) {
  const size_t rem = sizeof(self->chars) - self->index;
  return rem;
}

inline int strb_p(strb *self, const char *str) {
  const size_t remaining = strb_rem(self);
  const int n = snprintf(self->chars + self->index, remaining, "%s", str);
  if (n < 0) {
    return -1;
  }
  self->index += n;
  const size_t remaining2 = strb_rem(self);
  if (remaining2 < 1) {
    return -2;
  }
  return 0;
}

int strb_p_char(strb *self, char ch) {
  if (strb_rem(self) < 2) {
    return -1;
  }
  self->chars[self->index++] = ch;
  self->chars[self->index] = '\0';
  return 0;
}

inline int strb_p_long(strb *self, const long long num) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lld", num);
  strb_p(self, buf);
  return 0;
}

int strb_p_nbytes(strb *self, long long bytes) {
  const long long kb = bytes >> 10;
  if (kb == 0) {
    if (strb_p_long(self, bytes)) {
      return -1;
    }
    if (strb_p(self, " B")) {
      return -2;
    }
    return 0;
  }
  const long long mb = kb >> 10;
  if (mb == 0) {
    if (strb_p_long(self, kb)) {
      return -3;
    }
    if (strb_p(self, " KB")) {
      return -4;
    }
    return 0;
  }
  if (strb_p_long(self, mb)) {
    return -5;
  }
  if (strb_p(self, " MB")) {
    return -6;
  }
  return 0;
}

void strb_back(strb *self) {
  if (self->index > 0) {
    self->index--;
    self->chars[self->index] = '\0';
  }
}

void strb_clear(strb *self) {
  self->index = 0;
  self->chars[0] = '\0';
}
