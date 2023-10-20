#ifndef STRB_h
#define STRB_h
#include <unistd.h>

typedef struct strb {
  char chars[512];
  // index in chars. points at string terminator '\0'
  size_t index;
} strb;

// initiates self
void strb_init(strb *self);

// returns remaining free chars in self
size_t strb_rem(strb *self);

// appends str to self. returns 0 if ok
int strb_p(strb *self, const char *str);

// appends ch to self. returns 0 if oks
int strb_p_char(strb *self, char ch);

// appends num to str. returns 0 if ok
int strb_p_int(strb *self, const int num);

// appends num to str with specified field width aligned right
// returns 0 if ok
int strb_p_int_with_width(strb *self, int num, int width);

// appends num to str. returns 0 if ok
int strb_p_long(strb *self, long long num);

// formats bytes to str. returns 0 if ok
int strb_p_nbytes(strb *self, long long bytes);

// back and write '\0'
void strb_back(strb *self);

// clears buffer by setting index to 0 and first char to EOS
void strb_clear(strb *self);

#endif
