#include <sys/types.h>
#include <unistd.h>
#include "strerr.h"
#include "pathexec.h"

#define USAGE " 0|6"
#define WARNING "init: warning: "
#define FATAL "init: fatal: "

#define RUNIT "/sbin/runit"
#define HALT "/sbin/runit-halt"
#define REBOOT "/sbin/runit-reboot"

const char *progname;

void usage(void) {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

int main (int argc, const char * const *argv, const char * const *envp) {
  const char *prog[2];

  progname =*argv++;

  prog[1] =0;

  if (getpid() == 1) {
    prog[0] ="runit";

    /* kernel is starting init, runit does the job. */
    pathexec_run(RUNIT, (const char* const *) prog, envp);

    /* serious error */
    strerr_die4sys(111, FATAL, "unable to start ", prog[0], ": ");
  }

  if (! *argv) usage();

  if (**argv == '0') {
    prog[0] =HALT;
    pathexec_run(*prog, (const char* const *) prog, envp);
    strerr_die4sys(111, FATAL, "unable to start ", prog[0], ": ");
  }
  if (**argv == '6') {
    prog[0] =REBOOT;
    pathexec_run(*prog, (const char* const *) prog, envp);
    strerr_die4sys(111, FATAL, "unable to start ", prog[0], ": ");
  }

  usage();
  
  /* not reached */
  exit(0);
}
