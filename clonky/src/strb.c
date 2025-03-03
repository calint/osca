//
// reviewed: 2025-02-24
//
#include "strb.h"
#include <stdio.h>

inline void strb_init(strb *self) {
  self->index = 0;
  self->chars[0] = '\0';
}

inline uint32_t strb_rem(strb *self) {
  return sizeof(self->chars) - self->index;
}

inline int strb_p(strb *self, const char *str) {
  const uint32_t remaining = strb_rem(self);
  const int n = snprintf(self->chars + self->index, remaining, "%s", str);
  if (n < 0) {
    return -1;
  }

  self->index += (uint32_t)n > remaining - 1 ? remaining - 1 : (uint32_t)n;
  // note: -1 is for the '\0' that is not included in 'n' but is included in
  // 'remaining'

  return (uint32_t)n < remaining ? 0 : -2;
}

inline int strb_p_char(strb *self, char ch) {
  if (strb_rem(self) < 2) {
    return -1;
  }
  self->chars[self->index++] = ch;
  self->chars[self->index] = '\0';
  return 0;
}

inline int strb_p_int64(strb *self, const int64_t num) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%ld", num);
  return strb_p(self, buf);
}

inline int strb_p_uint64(strb *self, const uint64_t num) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%lu", num);
  return strb_p(self, buf);
}

inline int strb_p_int32(strb *self, const int32_t num) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%d", num);
  return strb_p(self, buf);
}

inline int strb_p_uint32(strb *self, const uint32_t num) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%u", num);
  return strb_p(self, buf);
}

inline int strb_p_int32_with_width(strb *self, const int32_t num,
                                   const uint32_t width) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%*d", width, num);
  return strb_p(self, buf);
}

inline int strb_p_uint32_with_width(strb *self, const uint32_t num,
                                    const uint32_t width) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%*u", width, num);
  return strb_p(self, buf);
}

int strb_p_nbytes(strb *self, const uint64_t nbytes) {
  const uint64_t kb = (nbytes + 512) >> 10;
  // note: +512 is for rounding
  if (kb == 0) {
    if (strb_p_uint64(self, nbytes)) {
      return -1;
    }
    if (strb_p(self, " B")) {
      return -2;
    }
    return 0;
  }
  const uint64_t mb = (kb + 512) >> 10;
  // note: +512 is for rounding
  if (mb == 0) {
    if (strb_p_uint64(self, kb)) {
      return -3;
    }
    if (strb_p(self, " KB")) {
      return -4;
    }
    return 0;
  }
  const uint64_t gb = (mb + 512) >> 10;
  // note: +512 is for rounding
  if (gb == 0) {
    if (strb_p_uint64(self, mb)) {
      return -5;
    }
    if (strb_p(self, " MB")) {
      return -6;
    }
    return 0;
  }

  if (strb_p_uint64(self, gb)) {
    return -7;
  }
  if (strb_p(self, " GB")) {
    return -8;
  }

  return 0;
}

void strb_back(strb *self) {
  if (self->index > 0) {
    self->index--;
    self->chars[self->index] = '\0';
  }
}
