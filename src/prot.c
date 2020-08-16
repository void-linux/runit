/* Public domain. */

#include "hasshsgr.h"
#include "prot.h"

#include <sys/types.h>
#include <unistd.h>
#include <grp.h>

int prot_gid(gid_t gid)
{
#ifdef HASSHORTSETGROUPS
  short x[2];
  x[0] = gid; x[1] = 73; /* catch errors */
  if (setgroups(1,x) == -1) return -1;
#else
  if (setgroups(1,&gid) == -1) return -1;
#endif
  return setgid(gid); /* _should_ be redundant, but on some systems it isn't */
}

int prot_uid(uid_t uid)
{
  return setuid(uid);
}
