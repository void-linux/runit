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

const char * const stage[3] ={
  "/etc/runit/1",
  "/etc/runit/2",
  "/etc/runit/3" };

int selfpipe[2];
int sigc =0;
int sigi =0;

void sig_cont_handler (void) {
  sigc++;
  write(selfpipe[1], "", 1);
}
void sig_int_handler (void) {
  sigi++;
  write(selfpipe[1], "", 1);
}
void sig_child_handler (void) {
  write(selfpipe[1], "", 1);
}

int main (int argc, const char * const *argv, const char * const *envp) {
  int pid, pid2;
  int wstat;
  int fd;
  int st;
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

  strerr_warn3(INFO, "$Id$",
	       ": booting.", 0);

  /* runit */
  for (st =0; st < 3; st++) {
    while ((pid =fork()) == -1) {
      strerr_warn4(FATAL, "unable to fork for \"", stage[st], "\" pausing: ",
		   &strerr_sys);
      sleep(5);
    }
    if (!pid) {
      /* child */
      const char * prog[2];

      prog[0] =stage[st];
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
            
      strerr_warn3(INFO, "enter stage: ", stage[st], 0);
      pathexec_run(*prog, (const char* const *) prog, envp);
      strerr_warn3(FATAL, "could not start child: ", stage[st], &strerr_sys);
    }

    x.fd =selfpipe[0];
    x.events =IOPAUSE_READ;

    for (;;) {
      int child;

      sig_unblock(sig_child);
      sig_unblock(sig_cont);
      sig_unblock(sig_int);
      poll(&x, 1, -1);
      sig_block(sig_cont);
      sig_block(sig_child);
      sig_block(sig_int);
      
      read(selfpipe[0], &ch, 1);
      
      if ((child =wait_nohang(&wstat)) == pid) {
	if (wait_crashed(wstat)) {
	  strerr_warn3(WARNING, "child crashed: ", stage[st], 0);
	  if (st == 1) {
	    /* this is stage 2 */
	    strerr_warn2(WARNING, "killing all processes in stage 2...", 0);
	    kill(-pid, 9);
	    sleep(5);
	    strerr_warn2(WARNING, "restarting.", 0);
	    st--;
	    break;
	  }
	}
	strerr_warn3(INFO, "leave stage: ", stage[st], 0);
	break;
      }
      if (child > 0) {
	/* collect terminated children */
	write(selfpipe[1], "", 1);
	continue;
      }

      /* sig */
      if (!sigc  && !sigi) {
#ifdef DEBUG
	strerr_warn2(WARNING, "poll: ", &strerr_sys);
#endif
	continue;
      }
      if (st != 1) {
	strerr_warn2(WARNING, "sigs only work in stage 2.", 0);
	sigc =sigi =0;
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
	sigi =0;
	sigc++;
      }
      if (sigc && ((fd =open_write(STOPIT)) != -1)) {
	int i;
	close(fd);
	unlink(STOPIT);

	/* kill stage 2 */
#ifdef DEBUG
	strerr_warn2(WARNING, "sending sigterm...", 0);
#endif
	kill(pid, sig_term);
	i =0;
	while (i < 5) {
	  if ((child =wait_nohang(&wstat)) == pid) {
#ifdef DEBUG
	    strerr_warn2(WARNING, "stage 2 terminated.", 0);
#endif
	    pid =0;
	    break;
	  }
	  if (child) continue;
	  if (child == -1) {
	    strerr_warn2(WARNING, "wait_nohang: ", &strerr_sys);
	  }
#ifdef DEBUG
	  strerr_warn2(WARNING, "waiting...", 0);
#endif
	  sleep(1);
	  i++;
	}
	if (pid) {
	  /* still there */
	  strerr_warn2(WARNING,
		       "stage 2 not terminated, sending sigkill...", 0);
	  kill(pid, 9);
	  if (wait_pid(&wstat, pid) == -1) {
	    strerr_warn2(WARNING, "wait_pid: ", &strerr_sys);
	  }
	}
	sigc =0;

	/* enter stage 3 */
	break;
      }
      sigc =sigi =0;

#ifdef DEBUG
      strerr_warn2(WARNING, "no request.", 0);
#endif
    }
  }

  /* not reached */
  strerr_die2x(0, INFO, "exit.");
  return(0);
}
