/* Public domain. */

#include "byte.h"

void byte_copy(register char *to, register unsigned int n, register char *from)
{
  for (;;) {
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
    if (!n) return; *to++ = *from++; --n;
  }
}
