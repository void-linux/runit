#include "fmt_ptime.h"
#include "fmt.h"

unsigned int fmt_ptime(char *s, struct taia *ta) {
  struct tm *t;

  if (! (t =localtime((time_t*)&ta->sec.x))) return(0);
  fmt_ulong(s, 1900 +t->tm_year);
  s[4] ='-'; fmt_uint0(&s[5], t->tm_mon +1, 2);
  s[7] ='-'; fmt_uint0(&s[8], t->tm_mday, 2);
  s[10] ='_'; fmt_uint0(&s[11], t->tm_hour, 2);
  s[13] =':'; fmt_uint0(&s[14], t->tm_min, 2);
  s[16] =':'; fmt_uint0(&s[17], t->tm_sec, 2);
  s[19] ='.'; fmt_uint0(&s[20], ta->nano, 9);
  return(25);
}

unsigned int fmt_taia(char *s, struct taia *t) {
  static char hex[16] ="0123456789abcdef";
  static char pack[TAIA_PACK];
  int i;

  taia_pack(pack, t);
  s[0] ='@';
  for (i =0; i < 12; ++i) {
    s[i *2 +1] =hex[(pack[i] >>4) &15];
    s[i *2 +2] =hex[pack[i] &15];
  }
  return(25);
}
