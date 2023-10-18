#include "strb.h"
#include <stdio.h>

inline void strb_init(strb *o) { o->index = 0; }

inline size_t strb_rem(strb *o) {
  const long long remaining = (long long)((sizeof o->chars) - o->index);
  return remaining;
}

inline int strb_p(strb *o, const char *str) {
  const size_t remaining = strb_rem(o);
  const int n = snprintf(o->chars + o->index, remaining, "%s", str);
  if (n < 0)
    return -1;
  o->index += n;
  const size_t remaining2 = strb_rem(o);
  if (remaining2 < 1)
    return -2;
  //	printf("index:%zd  remaining:%zd
  // string:'%s'\n",o->index,remaining2,o->chars);
  return 0;
}
inline int strb_fmt_long(strb *o, const long long n) {
  char s[32];
  snprintf(s, sizeof s, "%lld", n);
  strb_p(o, s);
  return 0;
}
int strb_fmt_bytes(strb *o, long long bytes) {
  const long long kb = bytes >> 10;
  if (kb == 0) {
    if (strb_fmt_long(o, bytes))
      return -1;
    if (strb_p(o, " B"))
      return -2;
    return 0;
  }
  const long long mb = kb >> 10;
  if (mb == 0) {
    if (strb_fmt_long(o, kb))
      return -3;
    if (strb_p(o, " KB"))
      return -4;
    return 0;
  }
  if (strb_fmt_long(o, mb))
    return -5;
  if (strb_p(o, " MB"))
    return -6;
  return 0;
}

/// clears buffer by setting index to 0 and first char to EOS
void strb_clear(strb *o) {
  o->index = 0;
  o->chars[0] = '\0';
}
