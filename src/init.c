#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include "strerr.h"
#include "pathexec.h"
#include "sig.h"
#include "open.h"
#include "error.h"

#define USAGE " 0|6"
#define WARNING "init: warning: "
#define FATAL "init: fatal: "

#define RUNIT "/sbin/runit"
#define STOPIT "/etc/runit/stopit"
#define REBOOT "/etc/runit/reboot"

const char *progname;

void usage(void) {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

void runit_halt () {
  if (open_trunc(STOPIT) == -1) {
    strerr_die4sys(111, FATAL, "unable to create ", STOPIT, ": ");
  }
  if (unlink(REBOOT) == -1) {
    if (errno != error_noent) {
      strerr_die4sys(111, FATAL, "unable to remove ", REBOOT, ": ");
    }
  }
  kill(1, sig_cont);
  exit(0);
}
void runit_reboot () {
  if (open_trunc(STOPIT) == -1) {
    strerr_die4sys(111, FATAL, "unable to create ", STOPIT, ": ");
  }
  if (open_trunc(REBOOT) == -1) {
    strerr_die4sys(111, FATAL, "unable to create ", REBOOT, ": ");
  }
  kill(1, sig_cont);
  exit(0);
}

int main (int argc, const char * const *argv, const char * const *envp) {
  const char *prog[2];

  progname =*argv++;

  if (getpid() == 1) {
    prog[1] =0;
    prog[0] ="runit";

    /* kernel is starting init, runit does the job. */
    pathexec_run(RUNIT, (const char * const *) prog, envp);

    /* serious error */
    strerr_die4sys(111, FATAL, "unable to start ", prog[0], ": ");
  }

  if (! *argv || ! **argv) usage();

  switch (**argv) {
  case '0':
    runit_halt();
    break;
  case '6':
    runit_reboot();
    break;
  case '-':
    if ((*argv)[1] == 'V') {
      strerr_warn1("$Id$\n", 0);
    }
  default:
    usage();
  }
  
  /* not reached */
  exit(0);
}
