#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include "strerr.h"
#include "error.h"
#include "taia.h"
#include "sig.h"
#include "env.h"
#include "coe.h"
#include "ndelay.h"
#include "fifo.h"
#include "open.h"
#include "lock.h"
#include "iopause.h"
#include "wait.h"
#include "fd.h"
#include "buffer.h"
#include "fmt.h"

#define USAGE " dir"

#define VERSION "$Id$"

char *progname;
int selfpipe[2];

/* state */
#define S_DOWN 0
#define S_RUN 1
#define S_FINISH 2
/* ctrl */
#define C_NOOP 0
#define C_TERM 1
#define C_PAUSE 2
/* want */
#define W_UP 0
#define W_DOWN 1
#define W_EXIT 2

struct svdir {
  int pid;
  int state;
  int ctrl;
  int want;
  struct taia start;
  int fdlock;
  int fdcontrol;
  int fdcontrolwrite;
  int islog;
};
struct svdir svd[2];

int haslog =0;
int logpipe[2];
char *dir;

void usage () {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}
void fatal(char *m) {
  strerr_die5sys(111, "runsv ", dir, ": fatal: ", m, ": ");
}
void fatal2(char *m1, char *m2) {
  strerr_die6sys(111, "runsv ", dir, ": fatal: ", m1, m2, ": ");
}
void warn(char *m) {
  strerr_warn5("runsv ", dir, ": warning: ", m, ": ", &strerr_sys);
}
void warnx(char *m) {
  strerr_warn4("runsv ", dir, ": warning: ", m, 0);
}

void stopservice(struct svdir *);

void s_child() {
  write(selfpipe[1], "", 1);
}
void s_term() {
  svd[0].want =W_EXIT;
  stopservice(&svd[0]);
  write(selfpipe[1], "", 1);
}

void update_status(struct svdir *s) {
  unsigned long l;
  int fd;
  char status[19];
  char bspace[64];
  buffer b;
  char spid[FMT_ULONG];
  
  if (s->islog) {
    if ((fd =open_trunc("log/supervise/state")) == -1)
      fatal("unable to open log/supervise/state");
  }
  else {
    if ((fd =open_trunc("supervise/state")) == -1)
      fatal("unable to open supervise/state");
  }
  buffer_init(&b, buffer_unixwrite, fd, bspace, sizeof bspace);
  spid[fmt_ulong(spid, (unsigned long)s->pid)] =0;
  switch (s->state) {
  case S_DOWN:
    buffer_puts(&b, "down");
    break;
  case S_RUN:
    buffer_puts(&b, "run");
    break;
  case S_FINISH:
    buffer_puts(&b, "finish");
    break;
  }
  if (s->pid) {
    buffer_puts(&b, " pid (");
    buffer_puts(&b, spid);
    buffer_puts(&b, ")");
  }
  if (s->ctrl & C_PAUSE)
    buffer_puts(&b, ", paused");
  if (s->ctrl & C_TERM)
    buffer_puts(&b, ", got TERM");
  switch(s->want) {
  case W_DOWN:
    if (s->state != S_DOWN) buffer_puts(&b, ", want down");
    break;
  case W_EXIT:
    buffer_puts(&b, ", want exit");
    break;
  }
  buffer_putsflush(&b, "\n");
  close(fd);

  /* supervise compatibility */
  taia_pack(status, &s->start);
  l =(unsigned long)s->pid;
  status[12] =l; l >>=8;
  status[13] =l; l >>=8;
  status[14] =l; l >>=8;
  status[15] =l;
  if (s->ctrl & C_PAUSE)
    status[16] =1;
  else
    status[16] =0;
  if (s->want == W_UP)
    status[17] ='u';
  else
    status[17] ='d';
  if (s->ctrl & C_TERM)
    status[18] =1;
  else
    status[18] =0;
  if ((fd =open_trunc("supervise/status.new")) == -1) {
    warn("unable to open supervise/status.new");
    return;
  }
  if ((l =write(fd, status, sizeof status)) == -1) {
    warn("unable to write supervise/status.new");
    close(fd);
    unlink("supervise/status.new");
    return;
  }
  close(fd);
  if (l < sizeof status) {
    warnx("unable to write supervise/status.new: partial write.");
    return;
  }
  if (s->islog) {
    if (rename("supervise/status.new", "log/supervise/status") == -1)
      warn("unable to rename supervise/status.new to log/supervise/status");
  }
  else {
    if (rename("supervise/status.new", "supervise/status") == -1)
      warn("unable to rename supervise/status.new to supervise/status");
  }
}

void stopservice(struct svdir *s) {
  if (s->pid) kill(s->pid, SIGTERM);
  s->ctrl |=C_TERM;
  update_status(s);
}

void startservice(struct svdir *s) {
  int p;
  char *run[2];
  
  if (s->state == S_FINISH)
    run[0] ="./finish";
  else
    run[0] ="./run";
  run[1] =0;

  if (s->pid != 0) stopservice(s); /* should never happen */
  while ((p =fork()) == -1) {
    warn("unable to fork, sleeping");
    sleep(5);
  }
  if (p == 0) {
    /* child */
    if (haslog) {
      if (s->islog) {
	if (fd_copy(0, logpipe[0]) == -1)
	  fatal("unable to setup filedescriptor for ./log/run");
	if (chdir("./log") == -1)
	  fatal("unable to change directory ./log");
      }
      else {
	if (fd_copy(1, logpipe[1]) == -1)
	  fatal("unable to setup filedescriptor for ./run");
      }
    }
    sig_uncatch(sig_child);
    sig_unblock(sig_child);
    sig_uncatch(sig_term);
    sig_unblock(sig_term);
    execve(*run, run, environ);
    if (s->islog)
      fatal2("unable to start log/", *run);
    else
      fatal2("unable to start ", *run);
  }
  if (s->state != S_FINISH)
    taia_now(&s->start);
  s->pid =p;
  s->ctrl =C_NOOP;
  if (s->state != S_FINISH) s->state =S_RUN;
  update_status(s);
  sleep(1);
}

int ctrl(struct svdir *s, char c) {
  switch(c) {
  case 'd': /* down */
    s->want =W_DOWN;
    if (s->pid && s->state != S_FINISH) stopservice(s);
    else update_status(s);
    break;
  case 'u': /* up */
    s->want =W_UP;
    if (s->pid == 0) startservice(s);
    else update_status(s);
    break;
  case 'x': /* exit */
    if (s->islog) break;
    s->want =W_EXIT;
    if (s->pid && s->state != S_FINISH)
      stopservice(s);
    break;
  case 't': /* sig term */
    if (s->pid && s->state != S_FINISH) stopservice(s);
    break;
  case 'k': /* sig kill */
    if (s->pid) kill(s->pid, SIGKILL);
    s->state =S_DOWN;
    break;
  case 'p': /* sig pause */
    kill(s->pid, SIGSTOP);
    s->ctrl |=C_PAUSE;
    update_status(s);
    break;
  case 'c': /* sig cont */
    kill(s->pid, SIGCONT);
    if (s->ctrl & C_PAUSE)
      s->ctrl &=~C_PAUSE;
    update_status(s);
    break;
  case 'o': /* once */
    s->want =W_DOWN;
    if (! s->pid) startservice(s);
    else update_status(s);
    break;
  case 'a': /* sig alarm */
    if (s->pid) kill(s->pid, SIGALRM);
    break;
  case 'h': /* sig hup */
    if (s->pid) kill(s->pid, SIGHUP);
    break;
  case 'i': /* sig int */
    if (s->pid) kill(s->pid, SIGINT);
    break;
  }
  return(1);
}

int main(int argc, char **argv) {
  struct stat s;
  int fd;

  progname =argv[0];
  if (! argv[1] || argv[2]) usage();
  dir =argv[1];

  if (pipe(selfpipe) == -1)
    fatal("unable to create selfpipe");
  coe(selfpipe[0]);
  coe(selfpipe[1]);
  ndelay_on(selfpipe[0]);
  ndelay_on(selfpipe[1]);
  
  sig_block(sig_child);
  sig_catch(sig_child, s_child);
  sig_block(sig_term);
  sig_catch(sig_term, s_term);

  if (chdir(dir) == -1)
    fatal("unable to change to directory");
  svd[0].pid =0;
  svd[0].state =S_DOWN;
  svd[0].ctrl =C_NOOP;
  svd[0].want =W_UP;
  svd[0].islog =0;
  taia_now(&svd[0].start);
  if (stat("down", &s) != -1)
    svd[0].want =W_DOWN;

  if (stat("log", &s) == -1) {
    if (errno != error_noent)
      warn("unable to stat() ./log: ");
  }
  else {
    if (! S_ISDIR(s.st_mode))
      warnx("./log: not a directory.");
    else {
      haslog =1;
      svd[1].pid =0;
      svd[1].state =S_DOWN;
      svd[1].ctrl =C_NOOP;
      svd[1].want =W_UP;
      svd[1].islog =1;
      taia_now(&svd[1].start);
      if (stat("log/down", &s) != -1)
	svd[1].want =W_DOWN;
    }
  }

  if (haslog) {
    if (pipe(logpipe) == -1)
      fatal("unable to create log pipe");
    coe(logpipe[0]);
    coe(logpipe[1]);
  }

  mkdir("supervise", 0700);
  if ((svd[0].fdlock =open_append("supervise/lock")) == -1)
    fatal("unable to open lock.");
  if (lock_exnb(svd[0].fdlock) == -1)
    fatal("unable to lock.");
  if (haslog) {
    mkdir("log/supervise", 0700);
    if ((svd[1].fdlock =open_append("log/supervise/lock")) == -1)
      fatal("unable to open log/lock.");
    if (lock_ex(svd[1].fdlock) == -1)
      fatal("unable to log/lock.");
  }

  fifo_make("supervise/control", 0600);
  if ((svd[0].fdcontrol =open_read("supervise/control")) == -1)
    fatal("unable to open supervise/control");
  coe(svd[0].fdcontrol);
  if ((svd[0].fdcontrolwrite =open_write("supervise/control")) == -1)
    fatal("unable to open supervise/control");
  coe(svd[0].fdcontrolwrite);
  update_status(&svd[0]);
  if (haslog) {
    fifo_make("log/supervise/control", 0600);
    if ((svd[1].fdcontrol =open_read("log/supervise/control")) == -1)
      fatal("unable to open log/supervise/control");
    coe(svd[1].fdcontrol);
    if ((svd[1].fdcontrolwrite =open_write("log/supervise/control")) == -1)
      fatal("unable to open log/supervise/control");
    coe(svd[1].fdcontrolwrite);
    update_status(&svd[1]);
  }
  fifo_make("supervise/ok",0600);
  if ((fd =open_read("supervise/ok")) == -1)
    fatal("unable to read supervise/ok");
  coe(fd);
  if (haslog) {
    fifo_make("log/supervise/ok",0600);
    if ((fd =open_read("log/supervise/ok")) == -1)
      fatal("unable to read log/supervise/ok");
    coe(fd);
  }
  for (;;) {
    iopause_fd x[3];
    struct taia deadline;
    struct taia now;
    char ch;

    if (haslog)
      if (! svd[1].pid && svd[1].want == W_UP) startservice(&svd[1]);
    if (! svd[0].pid && svd[0].want == W_UP) startservice(&svd[0]);

    x[0].fd =selfpipe[0];
    x[0].events =IOPAUSE_READ;
    x[1].fd =svd[0].fdcontrol;
    x[1].events =IOPAUSE_READ;
    if (haslog) {
      x[2].fd =svd[1].fdcontrol;
      x[2].events =IOPAUSE_READ;
    }
    taia_now(&now);
    taia_uint(&deadline, 3600);
    taia_add(&deadline, &now, &deadline);

    sig_unblock(sig_term);
    sig_unblock(sig_child);
    iopause(x, 2 +haslog, &deadline, &now);
    sig_block(sig_term);
    sig_block(sig_child);

    while (read(selfpipe[0], &ch, 1) == 1)
      ;
    for (;;) {
      int child;
      int wstat;
      
      child = wait_nohang(&wstat);
      if (!child) break;
      if ((child == -1) && (errno != error_intr)) break;
      if (child == svd[0].pid) {
	svd[0].pid =0;
	if (open_read("finish") != -1) {
	  svd[0].state =S_FINISH;
	  startservice(&svd[0]);
	  update_status(&svd[0]);
	  break;
	}
	svd[0].state =S_DOWN;
	svd[0].ctrl &=~C_TERM;
	taia_now(&svd[0].start);
	update_status(&svd[0]);
	if (svd[0].want == W_UP) {
	  startservice(&svd[0]);
	  break;
	}
      }
      if (haslog) {
	if (child == svd[1].pid) {
	  svd[1].pid =0;
	  svd[1].state =S_DOWN;
	  taia_now(&svd[1].start);
	  update_status(&svd[1]);
	  if (svd[1].want == W_UP) {
	    startservice(&svd[1]);
	    break;
	  }
	}
      }
    }
    if (read(svd[0].fdcontrol, &ch, 1) == 1)
      ctrl(&svd[0], ch);
    if (haslog) {
      if (read(svd[1].fdcontrol, &ch, 1) == 1)
	ctrl(&svd[1], ch);
    }

    if (svd[0].want == W_EXIT && svd[0].pid == 0) {
      if (svd[1].pid == 0)
	exit(0);
      if (svd[1].want != W_EXIT) {
	svd[1].want =W_EXIT;
	stopservice(&svd[1]);
      }
    }
  }
  exit(0);
}
