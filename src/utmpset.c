#include "strerr.h"

#define USAGE " line"
#define WARNING "utmpset: warning: "

const char *progname;

void usage(void) {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

int main (int argc, const char * const *argv, const char * const *envp) {
  progname =*argv++;
  if (! *argv || ! **argv) usage();
  if (logout(*argv) != 1)
    strerr_warn4(WARNING, "unable to logout line ", *argv, ": ", &strerr_sys);
  exit(0);
}
