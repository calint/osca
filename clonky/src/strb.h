#ifndef STRB_h
#define STRB_h

typedef struct strb {
  char chars[512];
  // index in chars. points at string terminator '\0'
  unsigned index;
} strb;
// note: declared in header because it is used on the stack

void strb_init(strb *self);

// returns remaining free chars
unsigned strb_rem(strb *self);

// appends 'str'
// returns 0 if ok
int strb_p(strb *self, const char *str);

// appends 'ch'
// returns 0 if ok
int strb_p_char(strb *self, char ch);

// formats and appends 'num'
// returns 0 if ok
int strb_p_int(strb *self, const int num);

// formats and appends 'num'
// returns 0 if ok
int strb_p_uint(strb *self, const unsigned num);

// appends 'num' with specified field 'width' aligned right
// returns 0 if ok
int strb_p_int_with_width(strb *self, int num, int width);

// appends 'num'
// returns 0 if ok
int strb_p_long(strb *self, long long num);

// formats number of bytes using suffixes (B, KB, MB, GB)
// returns 0 if ok
int strb_p_nbytes(strb *self, long long nbytes);

// backs one character and writes '\0'
void strb_back(strb *self);

// clears buffer by setting index to 0 and first char to '\0'
void strb_clear(strb *self);

#endif
