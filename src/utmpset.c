#include "strerr.h"
#include "sgetopt.h"
#define USAGE " [-w] line"
#define WARNING "utmpset: warning: "

const char *progname;

void usage(void) {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

int main (int argc, const char * const *argv, const char * const *envp) {
  int opt;
  int wtmp =0;

  progname =*argv;

  while ((opt =getopt(argc, argv, "wV")) != opteof) {
    switch(opt) {
    case 'w':
      wtmp =1;
      break;
    case 'V':
      strerr_warn1("$Id$", 0);
    case '?':
      usage();
    }
  }
  argv +=optind;

  if (! argv || ! *argv) usage();
  if (logout(*argv) != 1)
    strerr_die4sys(0, WARNING, "unable to logout line ", *argv, ": ");
  if (wtmp) logwtmp(*argv, "", "");
  exit(0);
}
