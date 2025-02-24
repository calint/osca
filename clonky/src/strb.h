#ifndef STRB_h
#define STRB_h

#include <stdint.h>

typedef struct strb {
  char chars[512];
  // index in chars. points at string terminator '\0'
  uint32_t index;
} strb;
// note: declared in header because it is used on the stack

void strb_init(strb *self);

// returns remaining free chars
uint32_t strb_rem(strb *self);

// appends 'str'
// returns 0 if ok
int strb_p(strb *self, const char *str);

// appends 'ch'
// returns 0 if ok
int strb_p_char(strb *self, char ch);

// formats and appends 'num'
// returns 0 if ok
int strb_p_int32(strb *self, const int32_t num);

// formats and appends 'num'
// returns 0 if ok
int strb_p_uint32(strb *self, const uint32_t num);

// appends 'num' with specified field 'width' aligned right
// returns 0 if ok
int strb_p_int32_with_width(strb *self, int32_t num, uint32_t width);

// appends 'num' with specified field 'width' aligned right
// returns 0 if ok
int strb_p_uint32_with_width(strb *self, uint32_t num, uint32_t width);

// appends 'num'
// returns 0 if ok
int strb_p_int64(strb *self, int64_t num);

int strb_p_uint64(strb *self, uint64_t num);

// formats number of bytes using suffixes (B, KB, MB, GB)
// returns 0 if ok
int strb_p_nbytes(strb *self, uint64_t nbytes);

// backs one character and writes '\0'
void strb_back(strb *self);

// clears buffer by setting index to 0 and first char to '\0'
void strb_clear(strb *self);

#endif
