#ifndef UIDGID_H
#define UIDGID_H

struct uidgid {
  int uid;
  int gid[61];
  int gids;
};

extern unsigned int uidgid_get(struct uidgid *, char *, unsigned int);

#endif
