#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include "direntry.h"
#include "strerr.h"
#include "error.h"
#include "wait.h"
#include "env.h"
#include "pathexec.h"
#include "fd.h"
#include "str.h"
#include "coe.h"
#include "iopause.h"
#include "sig.h"
#include "ndelay.h"

#define USAGE " dir"
#define VERSION "$Id$"

#define MAXSERVICES 1000

char *progname;
char *svdir;
struct {
  unsigned long dev;
  unsigned long ino;
  int pid;
  int isgone;
} sv[MAXSERVICES];
int svnum =0;
int check =1;
char *log =0;
int loglen;
int logpipe[2];
iopause_fd io[1];
struct taia stamplog;

void usage () {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}
void fatal(char *m1, char *m2) {
  strerr_die6sys(100, "runsvdir ", svdir, ": fatal: ", m1, m2, ": ");
}
void warn(char *m1, char *m2) {
  strerr_warn6("runsvdir ", svdir, ": warning: ", m1, m2, ": ", &strerr_sys);
}
void warn3x(char *m1, char *m2, char *m3) {
  strerr_warn6("runsvdir ", svdir, ": warning: ", m1, m2, m3, 0);
} 
void runsv(int no, char *name) {
  int pid;

  if ((pid =fork()) == -1) {
    warn("unable to fork for ", name);
    return;
  }
  if (pid == 0) {
    /* child */
    const char *prog[3];

    if (chdir(svdir) == -1)
      fatal("unable to change directory to", svdir);

    prog[0] ="runsv";
    prog[1] =name;
    prog[2] =0;
    if (log)
      if (fd_move(2, logpipe[1]) == -1)
	warn("unable to set filedescriptor for log service", 0);
    pathexec_run(*prog, prog, (const char* const*)environ);
    fatal("unable to start runsv ", name);
  }
  sv[no].pid =pid;
}

void runsvdir() {
  DIR *dir;
  direntry *d;
  int i;
  struct stat s;

  if (! (dir =opendir("."))) {
    warn("unable to open directory ", svdir);
    return;
  }
  for (i =0; i < svnum; i++)
    sv[i].isgone =1;
  errno =0;
  while ((d =readdir(dir))) {
    if (d->d_name[0] == '.') continue;
    if (stat(d->d_name, &s) == -1) {
      warn("unable to stat ", d->d_name);
      return;
    }
    if (! S_ISDIR(s.st_mode))
      continue;
    for (i =0; i < svnum; i++) {
      if ((sv[i].ino == s.st_ino) && (sv[i].dev == s.st_dev)) {
	sv[i].isgone =0;
	if (! sv[i].pid)
	  runsv(i, d->d_name);
	break;
      }
    }
    if (i == svnum) {
      /* new service */
      if (svnum >= MAXSERVICES) {
	warn3x("unable to start runsv ", d->d_name, ": too many services.");
	continue;
      }
      sv[i].ino =s.st_ino;
      sv[i].dev =s.st_dev;
      sv[i].pid =0;
      sv[i].isgone =0;
      svnum++;
      runsv(i, d->d_name);
    }
  }
  if (errno) {
    warn("unable to read directory ", svdir);
    closedir(dir);
    return;
  }
  closedir(dir);

  /* SIGTERM removed runsv's */
  for (i =0; i < svnum; i++) {
    if (! sv[i].isgone)
      continue;
    if (sv[i].pid)
      kill(sv[i].pid, SIGTERM);
    sv[i] =sv[--svnum];
    check =1;
  }
}

int setup_log() {
  if ((loglen =str_len(log)) < 7) {
    warn3x("log must have at least seven characters.", 0, 0);
    return(0);
  }
  if (pipe(logpipe) == -1) {
    warn3x("unable to create pipe for log.", 0, 0);
    return(-1);
  }
  coe(logpipe[1]);
  coe(logpipe[0]);
  ndelay_on(logpipe[0]);
  ndelay_on(logpipe[1]);
  if (fd_copy(2, logpipe[1]) == -1) {
    warn3x("unable to set filedescriptor for log.", 0, 0);
    return(-1);
  }
  io[0].fd =logpipe[0];
  io[0].events =IOPAUSE_READ;
  taia_now(&stamplog);
  return(1);
}

int main(int argc, char **argv) {
  struct stat s;
  time_t mtime =0;
  int wstat;
  int curdir;
  int pid;
  struct taia deadline;
  struct taia now;
  char ch;
  int i;

  progname =*argv++;
  if (! argv || ! *argv) usage();

  svdir =*argv++;
  if (argv && *argv) {
    log =*argv;
    if (setup_log() != 1) {
      log =0;
      warn3x("log service disabled.", 0, 0);
    }
  }
  if ((curdir =open_read(".")) == -1)
    fatal("unable to open current directory", 0);

  for (;;) {
    /* collect children */
    for (;;) {
      if ((pid =wait_nohang(&wstat)) <= 0) break;
      for (i =0; i < svnum; i++) {
	if (pid == sv[i].pid) {
	  /* runsv has gone */
	  sv[i].pid =0;
	  check =1;
	  break;
	}
      }
    }
    if (stat(svdir, &s) != -1) {
      if (check || s.st_mtime > mtime) {
	/* svdir modified */
	mtime =s.st_mtime;
	check =0;
        if (chdir(svdir) == -1)
	  warn("unable to change directory to", svdir);
	else {
	  runsvdir();
	  if (fchdir(curdir) == -1)
            warn("unable to change directory", 0);
        }
      }
    }
    else
      warn("unable to stat ", svdir);

    taia_now(&now);
    if (log)
      if (taia_less(&now, &stamplog) == 0) {
	write(logpipe[1], ".", 1);
	taia_uint(&deadline, 900);
	taia_add(&stamplog, &stamplog, &deadline);
      }
    taia_uint(&deadline, 5);
    taia_add(&deadline, &now, &deadline);

    sig_block(sig_child);
    if (log)
      iopause(io, 1, &deadline, &now);
    else
      iopause(0, 0, &deadline, &now);
    sig_unblock(sig_child);

    if (log)
      if (io[0].revents | IOPAUSE_READ) {
	while (read(logpipe[0], &ch, 1) > 0) {
	  if (ch) {
	    for (i =6; i < loglen; i++)
	      log[i -1] =log[i];
	    log[loglen -1] =ch;
	  }
	}
      }
  }
  /* not reached */
  exit(0);
}
