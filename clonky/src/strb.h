#ifndef STRB_h
#define STRB_h
#include <unistd.h>

typedef struct {
  char chars[512];
  size_t index;
} strb;

/// initiates @o
void strb_init(strb *self);

/// @returns remaining free chars in @o
size_t strb_rem(strb *self);

/// appends @str to @o  @returns 0 if ok
int strb_p(strb *self, const char *str);

/// appends @n to @o  @returns 0 if ok
int strb_fmt_long(strb *self, long long n);

/// formats @bytes to @o  @returns 0 if ok
int strb_fmt_bytes(strb *self, long long bytes);

/// clears buffer by setting index to 0 and first char to EOS
void strb_clear(strb *self);

#endif
