#include <unistd.h>
#include "strerr.h"
#include "error.h"
#include "open.h"

#define USAGE " u|d|o|x|p|c|h|a|i|t|k service ..."

#define VERSION "$Id$"

#define FATAL "rsv-control: fatal: "
#define WARNING "rsv-control: warning: "

char *progname;
unsigned int rc =0;

void usage() {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}
void fatal(char *m1) {
  strerr_die3sys(rc, FATAL, m1, ": ");
}
void warn(char *m1, char *m2) {
  rc++;
  strerr_warn5(WARNING, m1, ": ", m2, ": ", &strerr_sys);
}
void warnx(char *m1, char *m2) {
  rc++;
  strerr_warn4(WARNING, m1, ": ", m2, 0);
}

int ctrl(char *name, char c) {
  int fd;

  if ((fd =open_write("supervise/control")) == -1) {
    if (errno == error_nodevice)
      warnx(name, "supervise not running.");
    else
      warn(name, "unable to open supervise/control");
    return(-1);
  }
  if (write(fd, &c, 1) != 1) {
    warn(name, "unable to write to supervise/control");
    return(-1);
  }
  return(1);
}

int main(int argc, char **argv) {
  char **dir;
  int curdir;
  char c;

  progname =*argv++;

  if (! argv || ! *argv) usage();

  switch ((c =**argv)) {
  case 'u':
  case 'd':
  case 'o':
  case 'x':
  case 'p':
  case 'c':
  case 'h':
  case 'a':
  case 'i':
  case 't':
  case 'k':
    break;
  default:
    usage();
  }
  dir =++argv;
  if (! dir || ! *dir) usage();

  if ((curdir =open_read(".")) == -1) {
    rc =100;
    fatal("unable to open current directory");
  }
  for (; dir && *dir; dir++) {
    if (chdir(*dir) == -1) {
      warn(*dir, "unable to change directory");
      continue;
    }
    ctrl(*dir, c);
    if (fchdir(curdir) == -1) {
      rc =100;
      fatal("unable to change directory");
    }
  }
  exit(rc);
}
