#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include "uidgid.h"
#include "str.h"

unsigned int uidgid_get(struct uidgid *u, char *ug, unsigned int dogrp) {
  char *g =0;
  struct passwd *pwd =0;
  struct group *gr =0;
  int i, d =0;

  if (dogrp)
    if (ug[(d =str_chr(ug, ':'))] == ':') {
      ug[d] =0;
      g =ug +d +1;
    }
  if (! (pwd =getpwnam(ug))) { if (g) ug[d] =':'; return(0); }
  if (g) {
    ug[d] =':';
    for (i =0; i < 60; ++i) {
      if (g[(d =str_chr(g, ':'))] == ':') {
        g[d] =0;
        if (! (gr =getgrnam(g))) { g[d] =':'; return(0); }
        g[d] =':';
        u->gid[i] =gr->gr_gid;
        g +=d +1;
      }
      else {
        if (! (gr =getgrnam(g))) return(0);
        u->gid[i++] =gr->gr_gid;
        break;
      }
    }
    u->gid[i] =0;
    u->gids =i;
  }
  if (! g) { u->gid[0] =pwd->pw_gid; u->gids =1; }
  u->uid =pwd->pw_uid;
  return(1);
}
