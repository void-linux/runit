#include <unistd.h>

#include "strerr.h"
#include "error.h"
#include "pathexec.h"
#include "open.h"

#define INFO "utmpentry: "
#define WARNING "utmpentry: warning: "
#define FATAL "utmpentry: fatal: "

#define USAGE " [ -w ] prog"
#define UTMP "/var/run/utmp"
#define WTMP "var/log/wtmp"

const char *progname;

void usage () {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

int main (int argc, const char * const *argv, const char * const *envp) {
  int opt;
  int dowtmp =0;

progname =*argv;
  
  while ((opt =getopt(argc, argv, "wV")) != opteof) {
    switch(opt) {
    case 'w':
      dowtmp =1;
      break;
    case 'V':
      strerr_warn1("$Id", 0);
    case '?':
      usage();
    }
  }
  argv +=optind;
  if (! argv || ! *argv) {
    usage();
  }
  
  
  /* not reached */
  return(0);
}
