/* this requires: */
#include <poll.h>

#include <sys/types.h>
#include <sys/reboot.h>
#include <signal.h>
#include <unistd.h>

#include "sig.h"
#include "strerr.h"
#include "error.h"
#include "pathexec.h"
#include "iopause.h"
#include "coe.h"
#include "ndelay.h"
#include "wait.h"
#include "open.h"

/* #define DEBUG */

#define INFO "- runit: "
#define WARNING "- runit: warning: "
#define FATAL "- runit: fatal: "

#define STOPIT "/etc/runit/stopit"
#define CTRLALTDEL "/etc/runit/ctrlaltdel"

const char * const take[3] ={
  "/etc/runit/1",
  "/etc/runit/2",
  "/etc/runit/3" };
const char *progname;
int selfpipe[2];
int sigc =0;
int sigi =0;

void sig_cont_handler(void) {
  sigc++;
  write(selfpipe[1], "", 1);
}
void sig_int_handler(void) {
  sigi++;
  write(selfpipe[1], "", 1);
}
void sig_child_handler(void) {
  write(selfpipe[1], "", 1);
}

int main (int argc, const char * const *argv, const char * const *envp) {
  int pid, pid2;
  int wstat;
  int fd;
  int i;
  int stopit =0;
  iopause_fd x;
  char ch;

  setsid();

  sig_block(sig_alarm);
  sig_block(sig_child);
  sig_catch(sig_child, sig_child_handler);
  sig_block(sig_cont);
  sig_catch(sig_cont, sig_cont_handler);
  sig_block(sig_hangup);
  sig_block(sig_int);
  sig_catch(sig_int, sig_int_handler);
  sig_block(sig_pipe);
  sig_block(sig_term);

  /* create selfpipe */
  while (pipe(selfpipe) == -1) {
    strerr_warn2(FATAL, "unable to create selfpipe, pausing: ", &strerr_sys);
    sleep(5);
  }
  coe(selfpipe[0]);
  coe(selfpipe[1]);
  ndelay_on(selfpipe[0]);
  ndelay_on(selfpipe[1]);

  /* activate ctrlaltdel handling */
  reboot(0);

  strerr_warn3(INFO, "$Id$", ": booting.", 0);

  /* runit */
  for (i =0; i < 3; i++) {
    while ((pid =fork()) == -1) {
      strerr_warn4(FATAL, "unable to fork for \"", take[i], "\" pausing: ",
		   &strerr_sys);
      sleep(5);
    }
    if (!pid) {
      /* child */
      const char * prog[2];

      prog[0] =take[i];
      prog[1] =0;

      setsid();

      sig_unblock(sig_alarm);
      sig_unblock(sig_child);
      sig_uncatch(sig_child);
      sig_unblock(sig_cont);
      sig_ignore(sig_cont);
      sig_unblock(sig_hangup);
      sig_unblock(sig_int);
      sig_uncatch(sig_int);
      sig_unblock(sig_pipe);
      sig_unblock(sig_term);
            
      strerr_warn3(INFO, "enter stage: ", take[i], 0);
      pathexec_run(*prog, (const char* const *) prog, envp);
      strerr_warn3(FATAL, "could not start child: ", take[i], &strerr_sys);
    }

    x.fd =selfpipe[0];
    x.events =IOPAUSE_READ;

    for (;;) {
      sig_unblock(sig_child);
      sig_unblock(sig_cont);
      sig_unblock(sig_int);
      poll(&x, 1, -1);
      sig_block(sig_cont);
      sig_block(sig_child);
      sig_block(sig_int);

      while (read(selfpipe[0], &ch, 1) == 1) {}
      
      if (wait_nohang(&wstat) == pid) {
	if (wait_crashed(wstat) && !stopit) {
	  strerr_warn3(WARNING, "child crashed: ", take[i], 0);
	  if (i == 1) {
	    kill(-pid, SIGTERM);
	    sleep(1);
	    strerr_warn2(WARNING, "restarting.", 0);
	    i--;
	    break;
	  }
	}
	if (stopit) stopit =0;
	strerr_warn3(INFO, "leave stage: ", take[i], 0);
	break;
      }
      
      /* sig */
      if ((sigc == 0) && (sigi == 0)) {
#ifdef DEBUG
	strerr_warn2(WARNING, "poll: ", &strerr_sys);
#endif
	continue;
      }
      if ((i != 1) || stopit) {
	strerr_warn2(WARNING, "sigs only work in stage 2.", 0);
	sigc =sigi =0;
	continue;
      }
      if (sigc && ((fd =open_write(STOPIT)) != -1)) {
	close(fd);
	unlink(STOPIT);
	kill(pid, SIGTERM);
	sigc =sigi =0;
	stopit =1;
	continue;
      }
      if (sigi && ((fd =open_write(CTRLALTDEL)) != -1)) {
	close(fd);
	strerr_warn2(INFO, "ctrl-alt-del request...", 0);
	while ((pid2 =fork()) == -1) {
	  strerr_warn4(FATAL,
		       "unable to fork for \"", CTRLALTDEL, "\" pausing: ",
		       &strerr_sys);
	  sleep(5);
	}
	if (!pid2) {
	  /* child */
	  const char * prog[2];

	  prog[0] =CTRLALTDEL;
	  prog[1] =0;
            
	  strerr_warn3(INFO, "enter stage: ", prog[0], 0);
	  pathexec_run(*prog, (const char* const *) prog, envp);
	  strerr_warn3(FATAL, "could not start child: ", prog[0], &strerr_sys);
	}
	if (wait_pid(&wstat, pid2) == -1) {
	  strerr_warn2(FATAL, "wait_pid: ", &strerr_sys);
	}
	if (wait_crashed(wstat)) {
	  strerr_warn3(WARNING, "child crashed: ", CTRLALTDEL, 0);
	}
	kill(pid, SIGTERM);
	sigc =sigi =0;
	stopit =1;
	continue;
      }
      
#ifdef DEBUG
      strerr_warn2(WARNING, "no request.", 0);
#endif
    }
  }
  /* not reached */
  strerr_warn2(INFO, "exit.", 0);
  exit(0);
}
