#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "uidgid.h"
#include "str.h"

unsigned int uidgid_get(struct uidgid *u, char *ug, unsigned int dogrp) {
  char *g =0;
  struct passwd *pwd =0;
  struct group *gr =0;
  int d =0;

  if (dogrp)
    if (ug[(d =str_chr(ug, ':'))] == ':') {
      ug[d] =0;
      g =ug +d +1;
    }
  pwd =getpwnam(ug);
  if (g) {
    gr =getgrnam(g);
    ug[d] =':';
  }
  if (! pwd) return(0);
  if (g && ! gr) return(0);
  if (gr)
    u->gid =gr->gr_gid;
  else
    u->gid =pwd->pw_gid;
  u->uid =pwd->pw_uid;
  return(1);
}
