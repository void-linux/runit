#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "strerr.h"
#include "error.h"

#define USAGE " dir"

#define SVDIR "/etc/runit/runsvdir"

char *progname;
char *new;

void usage () {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}
void fatal(char *m1, char *m2) {
  strerr_die5sys(111, progname, ": fatal: ", m1, m2, ": ");
}
void fatalx(char *m1, char *m2) {
  strerr_die4x(111, progname, ": fatal: ", m1, m2);
}

int main (int argc, char **argv) {
  struct stat s;
  int dev;
  int ino;

  progname =*argv++;
  if (! argv || ! *argv) usage();

  new =*argv;
  if (new[0] == '.')
    fatalx(new, ": must not start with a dot.");
  if (chdir(SVDIR) == -1)
    fatal("unable to chdir: ", SVDIR);

  if (stat(new, &s) == -1) {
    if (errno == error_noent)
      fatal(new, 0);
    fatal("unable to stat: ", new);
  }
  if (! S_ISDIR(s.st_mode))
    fatalx(new, "not a directory.");
  ino =s.st_ino;
  dev =s.st_dev;
  if (stat("current", &s) == -1)
    fatal("unable to stat: ", "current");
  if ((s.st_ino == ino) && (s.st_dev == dev))
    strerr_die2x(0, new, ": already current.");

  if (unlink("current.new") == -1)
    if (errno != error_noent)
      fatal("unable to unlink: ", "current.new");
  if (symlink(new, "current.new") == -1)
    fatal("unable to create: current.new -> ", new);
  if (unlink("previous") == -1)
    if (errno != error_noent)
      fatal("unable to unlink: ", "previous");
  if (link("current", "previous") == -1)
    fatal("unable to copy: current to ", "previous");
  if (rename("current.new", "current") == -1)
    fatal("unabled to move: current.new to ", "current");

  strerr_die2x(0, new, ": now current.");
  exit(0);
}
