#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include "pmatch.h"
#include "fmt_ptime.h"
#include "alloc.h"
#include "stralloc.h"
#include "strerr.h"
#include "buffer.h"
#include "sig.h"
#include "env.h"
#include "fd.h"
#include "wait.h"
#include "error.h"
#include "sgetopt.h"
#include "open.h"
#include "openreadclose.h"
#include "coe.h"
#include "lock.h"
#include "str.h"
#include "byte.h"
#include "scan.h"
#include "direntry.h"
#include "taia.h"
#include "fmt.h"

#define USAGE " [-tv] [-r c] [-R abc] [-l n ] [-b n] dir ..."
#define VERSION "$Id$"

#define FATAL "svlogd: fatal: "
#define WARNING "svlogd: warning: "
#define PAUSE "svlogd: pausing: "
#define INFO "svlogd: info: "

const char *progname;

unsigned int verbose =0;
unsigned int timestamp =0;
unsigned long linelen =1000;
unsigned long buflen =1024;
const char *replace ="";
char repl =0;

const char **fndir;
int fdwdir;
struct stat st;
int i;
stralloc sa;
int wstat;
struct taia now;

char *databuf;
buffer data;
char *line;
char stamp[FMT_PTIME];
unsigned int exitasap =0;

struct logdir {
  int fddir;
  char *btmp;
  buffer b;
  stralloc inst;
  unsigned long size;
  unsigned long sizemax;
  unsigned long nmax;
  stralloc processor;
  int ppid;
  char fnsave[FMT_PTIME];
  char *name;
  int fdcur;
  int fdlock;
  unsigned int match;
} *dir;
unsigned int dirn =0;

void usage() { strerr_die4x(111, "usage: ", progname, USAGE, "\n"); }
void die_nomem() { strerr_die2x(111, FATAL, "out of memory."); }
void fatal(char *m0) { strerr_die3sys(111, FATAL, m0, ": "); }
void fatal2(char *m0, char *m1) {
  strerr_die5sys(111, FATAL, m0, ": ", m1, ": ");
}
void warn(char *m0) { strerr_warn3(WARNING, m0, ": ", &strerr_sys); }
void warn2(char *m0, char *m1) {
  strerr_warn5(WARNING, m0, ": ", m1, ": ", &strerr_sys);
}
void pause_nomem() { strerr_warn2(PAUSE, "out of memory.", 0); sleep(3); }
void pause1(char *m0) { strerr_warn3(PAUSE, m0, ": ", &strerr_sys); sleep(3); }
void pause2(char *m0, char *m1) {
  strerr_warn5(PAUSE, m0, ": ", m1, ": ", &strerr_sys);
  sleep(3);
}

int processorstart(struct logdir *ld) {
  int pid;

  if (! ld->processor.len) return(0);
  if (ld->ppid) {
    warn2("processor already running", ld->name);
    return(0);
  }
  while ((pid =fork()) == -1)
    pause2("unable to fork for processor", ld->name);
  if (! pid) {
    char *prog[4];
    int fd;

    /* child */
    sig_uncatch(sig_term);
    sig_uncatch(sig_alarm);
    sig_uncatch(sig_hangup);
    sig_unblock(sig_term);
    sig_unblock(sig_alarm);
    sig_unblock(sig_hangup);
    
    if ((fd =open_read(ld->fnsave)) == -1)
      fatal2("unable to open input for processor", ld->name);
    if (fd_move(0, fd) == -1)
      fatal2("unable to move filedescriptor for processor", ld->name);
    ld->fnsave[26] ='t';
    if ((fd =open_trunc(ld->fnsave)) == -1)
      fatal2("unable to open output for processor", ld->name);
    if (fd_move(1, fd) == -1)
      fatal2("unable to move filedescriptor for processor", ld->name);
    if ((fd =open_read("state")) == -1)
      fatal2("unable to open state for processor", ld->name);
    if (fd_move(4, fd) == -1)
      fatal2("unable to move filedescriptor for processor", ld->name);
    if ((fd =open_trunc("newstate")) == -1)
      fatal2("unable to open newstate for processor", ld->name);
    if (fd_move(5, fd) == -1)
      fatal2("unable to move filedescriptor for processor", ld->name);

    prog[0] = "sh";
    prog[1] = "-c";
    prog[2] = ld->processor.s;
    prog[3] = 0;
    execve("/bin/sh", prog, environ);
    fatal2("unable to run processor", ld->name);
  }
  ld->ppid =pid;
  return(1);  
}
int processorstop(struct logdir *ld) {
  char f[28];

  if (ld->ppid) {
    sig_unblock(sig_hangup);
    while (wait_pid(&wstat, ld->ppid) == -1)
      pause2("error waiting for processor", ld->name);
    sig_block(sig_hangup);
    ld->ppid =0;
  }
  while (fchdir(ld->fddir) == -1)
    pause2("unable to change directory, want processor", ld->name);
  if (wait_exitcode(wstat) != 0) {
    warn2("processor crashed, restart", ld->name);
    ld->fnsave[26] ='t';
    unlink(ld->fnsave);
    ld->fnsave[26] ='u';
    if (ld->processor.len) processorstart(ld);
    while (fchdir(fdwdir) == -1)
      pause1("unable to change to initial working directory");
    return(ld->processor.len ? 0 : 1);
  }
  if (unlink(ld->fnsave) == -1)
    strerr_warn5(WARNING, "unable to unlink: ", ld->name, "/", ld->fnsave, 0);
  ld->fnsave[26] ='t';
  byte_copy(f, 26, ld->fnsave);
  f[26] ='s'; f[27] =0;
  while (rename(ld->fnsave, f) == .1)
    pause2("unable to rename processed", ld->name);
  while (chmod(f, 0744) == -1)
    pause2("unable to set mode of processed", ld->name);
  while (fchdir(fdwdir) == -1)
    pause1("unable to change to initial working directory");
  return(1);
}

int rotate(struct logdir *ld) {
  DIR *d;
  direntry *f;
  int n =0;
  char tmp[FMT_ULONG +1];
  char oldest[FMT_PTIME];

  if (ld->size <= 0) return(1);
  if (ld->ppid) while(! processorstop(ld));

  while (fchdir(ld->fddir) == -1)
    pause2("unable to change directory, want rotate", ld->name);

  /* create new filename */
  ld->fnsave[25] ='.';
  if (ld->processor.len)
    ld->fnsave[26] ='u';
  else
    ld->fnsave[26] ='s';
  ld->fnsave[27] =0;
  do {
    taia_now(&now);
    fmt_taia(ld->fnsave, &now);
    errno =0;
  } while ((stat(ld->fnsave, &st) != -1) || (errno != error_noent));

  buffer_flush(&ld->b);
  while (fsync(ld->fdcur) == -1)
    pause2("unable to fsync current logfile", ld->name);
  while (fchmod(ld->fdcur, 0744) == -1)
    pause2("unable to set mode of current", ld->name);
  close(ld->fdcur);
  if (verbose) {
    tmp[0] =' '; tmp[fmt_ulong(tmp +1, ld->size) +1] =0;
    strerr_warn6(INFO, "rename: ", ld->name, "/current ",
		 ld->fnsave, tmp, 0);
  }
  while (rename("current", ld->fnsave) == -1)
    pause2("unable to rename current", ld->name);
  while ((ld->fdcur =open_append("current")) == -1)
    pause2("unable to create new current", ld->name);
  coe(ld->fdcur);
  ld->size =0;
  while (fchmod(ld->fdcur, 0644) == -1)
    pause2("unable to set mode of current", ld->name);
  buffer_init(&ld->b, buffer_unixwrite, ld->fdcur, ld->btmp, sizeof ld->btmp);

  oldest[0] ='A'; oldest[1] =oldest[27] =0; errno =0;
  while (! (d =opendir(".")))
    pause2("unable to open directory, want rotate", ld->name);
  while ((f =readdir(d)))
    if ((f->d_name[0] == '@') && (str_len(f->d_name) == 27)) {
      ++n;
      if (str_diff(f->d_name, oldest) < 0)
	byte_copy(oldest, 27, f->d_name);
    }
  if (errno) warn2("unable to read directory", ld->name);
  closedir(d);
  
  if (ld->nmax && (n >= ld->nmax)) {
    if (verbose)
      strerr_warn5(INFO, "delete: ", ld->name, "/", oldest, 0);
    if (*oldest && (unlink(oldest) == -1))
      warn2("unable to unlink oldest logfile", ld->name);
  }

  /*
    start processor
  */
  if (ld->processor.len) {
    processorstart(ld);
  }
  
  while (fchdir(fdwdir) == -1)
    pause1("unable to change to initial working directory");
  return(1);
}

void logdir_close(struct logdir *ld) {
  if (ld->fddir == -1) return;
  if (verbose) strerr_warn3(INFO, "close: ", ld->name, 0);
  close(ld->fddir);
  ld->fddir =-1;
  if (ld->fdcur) {
    buffer_flush(&ld->b);
    while (fsync(ld->fdcur) == -1)
      pause2("unable to fsync current logfile", ld->name);
    while (fchmod(ld->fdcur, 0744) == -1)
      pause2("unable to set mode of current", ld->name);
    close(ld->fdcur);
  }
  if (ld->fdlock) close(ld->fdlock);
  ld->fdlock =-1;
}

int logdir_open(struct logdir *ld, const char *fn) {
  if ((ld->fddir =open_read(fn)) == -1) {
    warn2("unable to open log directory", (char*)fn);
    return(0);
  }
  coe(ld->fddir);
  if (fchdir(ld->fddir) == -1) {
    logdir_close(ld);
    warn2("unable to change directory", (char*)fn);
    return(0);
  }
  ld->fdlock =open_append("lock");
  if ((ld->fdlock == -1) || (lock_exnb(ld->fdlock) == -1)) {
    logdir_close(ld);
    warn2("unable to lock directory", (char*)fn);
    while (fchdir(fdwdir) == -1)
      pause1("unable to change to initial working directory");
    return(0);
  }
  coe(ld->fdlock);

  ld->size =0;
  ld->sizemax =9000;
  ld->nmax =10;
  ld->name =(char*)fn;
  ld->match =0;
  while (! stralloc_copys(&ld->inst, "")) pause_nomem();
  while (! stralloc_copys(&ld->processor, "")) pause_nomem();

  /* read config */
  if ((i =openreadclose("config", &sa, 128)) == -1)
    warn2("unable to read config", ld->name);
  if (i != 0) {
    int len;
    
    if (verbose) strerr_warn4(INFO, "read: ", ld->name, "/config", 0);
    for (i =0; i < sa.len -1; ++i) {
      if ((len =byte_chr(&sa.s[i], sa.len -i, '\n')) == 1) {
	++i; continue;
      }
      sa.s[len +i] =0;
      switch(sa.s[i]) {
      case '\n':
      case '#':
	 break;
      case '+':
      case '-':
      case 'e':
      case 'E':
	while (! stralloc_catb(&ld->inst, &sa.s[i], len)) pause_nomem();
	while (! stralloc_0(&ld->inst)) pause_nomem();
	break;
      case 's':
	scan_ulong(&sa.s[i +1], &ld->sizemax);
	if (ld->sizemax < 2048) ld->sizemax =2048;
	break;
      case 'n':
	scan_ulong(&sa.s[i +1], &ld->nmax);
	break;
      case '!':
	while (! stralloc_copys(&ld->processor, &sa.s[i +1]))
	  pause_nomem();
	while (! stralloc_0(&ld->processor)) pause_nomem();
	break;
      }
      i +=len;
    }
  }

  /* open current */
  if ((i =stat("current", &st)) != -1) {
    if (st.st_size && ((st.st_mode & S_IRWXU) != S_IXUSR)) {
      ld->fnsave[25] ='.'; ld->fnsave[26] ='u'; ld->fnsave[27] =0;
      do {
	taia_now(&now);
	fmt_taia(ld->fnsave, &now);
	errno =0;
      } while ((stat(ld->fnsave, &st) != -1) || (errno != error_noent));
      while (rename("current", ld->fnsave) == -1)
	pause2("unable to rename current", ld->name);
    }
    else
      ld->size =st.st_size;
  }
  else
    if (errno != error_noent) {
      logdir_close(ld);
      warn2("unable to stat current", ld->name);
      while (fchdir(fdwdir) == -1)
	pause1("unable to change to initial working directory");
      return(0);
    }
  while ((ld->fdcur =open_append("current")) == -1)
    pause2("unable to open current", ld->name);
  coe(ld->fdcur);
  while (fchmod(ld->fdcur, 0644) == -1)
    pause2("unable to set mode of current", ld->name);
  if (! ld->btmp) 
    while (! (ld->btmp =(char*)alloc(buflen *sizeof(char)))) pause_nomem();
  buffer_init(&ld->b, buffer_unixwrite, ld->fdcur, ld->btmp, sizeof ld->btmp);
  
  if (verbose) {
    if (i == 0) strerr_warn4(INFO, "append: ", ld->name, "/current", 0);
    else strerr_warn4(INFO, "new: ", ld->name, "/current", 0);
  }
  
  while (fchdir(fdwdir) == -1)
    pause1("unable to change to initial working directory");
  return(1);
}

void logdirs_reopen(void) {
  int l;

  for (l =0; l < dirn; ++l) {
    logdir_close(&dir[l]);    
    logdir_open(&dir[l], fndir[l]);
  }
}

int linestart(struct logdir *ld, char *s, int len) {
  /* check inst, set match */
  ld->match ='+';
  if (ld->inst.len) {
    for (i =0; i < ld->inst.len; ++i) {
      switch(ld->inst.s[i]) {
      case '+':
      case '-':
	if (pmatch(&ld->inst.s[i +1], s, len))
	  ld->match =ld->inst.s[i];
	break;
      case 'e':
	if (pmatch(&ld->inst.s[i +1], s, len)) {
	  if (timestamp) buffer_puts(buffer_2, stamp);
	  buffer_put(buffer_2, s, len);
	  if (len == linelen) buffer_puts(buffer_2, "...");
	  buffer_putflush(buffer_2, "\n", 1);
	}
	break;
      case 'E':
	if (! pmatch(&ld->inst.s[i +1], s, len)) {
	  if (timestamp) buffer_puts(buffer_2, stamp);
	  buffer_put(buffer_2, s, len);
	  if (len == linelen) buffer_puts(buffer_2, "...");
	  buffer_putflush(buffer_2, "\n", 1);
	}
	break;
      }
      i +=byte_chr(&ld->inst.s[i], ld->inst.len -i, 0);
    }
  }
  if (ld->match == '-') return(0);
  if (timestamp) {
    buffer_puts(&ld->b, stamp);
    ld->size +=26;
  }
  buffer_put(&ld->b, s, len);
  ld->size +=len;
  return(1);
}
int lineadd(struct logdir *ld, char *s, int len) {
  if (ld->match != '+') return(0);
  buffer_put(&ld->b, s, len);
  ld->size +=len;
  if (ld->sizemax && (ld->size >= ld->sizemax)) rotate(ld);
  return(1);
}
int lineflush(struct logdir *ld, char *s, int len) {
  switch(ld->match) {
  case '-':
    return(0);
  case 0:
    linestart(ld, s, len);
    break;
  case '+':
    buffer_put(&ld->b, s, len);
    ld->size +=len;
    break;
  }
  if (ld->match == '+') {
    buffer_putflush(&ld->b, "\n", 1);
    ld->size +=1;
    ld->match =0;
    if (ld->sizemax && (ld->size >= (ld->sizemax -linelen))) rotate(ld);
    return(1);
  }
  return(0);
}
int buffer_pread(int fd, char *s, unsigned int len) {
  len =read(fd, s, len);
  if ((len == -1) && (errno == error_intr)) return(0);
  return(len);
}
void sig_term_handler(void) {
  exitasap =1;
  strerr_warn2(INFO, "sigterm received.", 0);
}
void sig_child_handler(void) {
  int pid, l;

  while ((pid =wait_nohang(&wstat)) > 0)
    for (l =0; l < dirn; ++l)
      if (dir[l].ppid == pid) {
	dir[l].ppid =0;
	processorstop(&dir[l]);
	break;
      }
  strerr_warn2(INFO, "sigchild received.", 0);
}
void sig_alarm_handler(void) {
  int l;
  
  for (l =0; l < dirn; ++l)
    if (dir[l].fddir != -1)
      if (dir[l].size > 0)
	rotate(&dir[l]);
  strerr_warn2(INFO, "sigalarm received.", 0);
}
void sig_hangup_handler(void) {
  logdirs_reopen();
  strerr_warn2(INFO, "sighangup received.", 0);
}

int main(int argc, const char **argv) {
  int opt;
  unsigned int eol;

  progname =*argv;

  while ((opt =getopt(argc, argv, "R:r:l:b:tvV")) != opteof) {
    switch(opt) {
    case 'R':
      replace =optarg;
      if (! repl) repl ='_';
      break;
    case 'r':
      repl =*optarg;
      if (! repl || *(optarg +1)) usage();
      break;
    case 'l':
      scan_ulong(optarg, &linelen);
      if (linelen == 0) linelen =1000;
      break;
    case 'b':
      scan_ulong(optarg, &buflen);
      if (buflen == 0) buflen =1024;
      break;
    case 't':
      ++timestamp;
      break;
    case 'v':
      ++verbose;
      break;
    case 'V': strerr_warn1(VERSION, 0);
    case '?': usage();
    }
  }
  argv +=optind;

  dirn =argc -optind;
  if (dirn <= 0) usage();
  if (buflen <= linelen) usage();
  if ((fdwdir =open_read(".")) == -1)
    fatal("unable to open current working directory");
  coe(fdwdir);
  dir =(struct logdir*)alloc(dirn *sizeof(struct logdir));
  if (! dir) die_nomem();
  for (i =0; i < dirn; ++i) {
    dir[i].fddir =-1;
    dir[i].btmp =0; /* grm */
  }
  databuf =(char*)alloc(buflen *sizeof(char));
  if (! databuf) die_nomem();
  buffer_init(&data, buffer_pread, 0, databuf, buflen);
  line =(char*)alloc(linelen *sizeof(char));
  if (! line) die_nomem();
  fndir =argv;

  sig_block(sig_term);
  sig_block(sig_child);
  sig_block(sig_alarm);
  sig_block(sig_hangup);
  sig_catch(sig_term, sig_term_handler);
  sig_catch(sig_child, sig_child_handler);
  sig_catch(sig_alarm, sig_alarm_handler);
  sig_catch(sig_hangup, sig_hangup_handler);

  logdirs_reopen();

  for(eol =0;;) {
    int r, len;
    char *ch;

    if (exitasap) {
      /* check for processors */
      for (i =0; i < dirn; ++i)
	if (dir[i].ppid) while (! processorstop(&dir[i]));
      break;
    }

    sig_unblock(sig_term);
    sig_unblock(sig_child);
    sig_unblock(sig_alarm);
    sig_unblock(sig_hangup);
    errno =0;
    r =buffer_feed(&data);
    sig_block(sig_term);
    sig_block(sig_child);
    sig_block(sig_alarm);
    sig_block(sig_hangup);

    if (r == -1) {
      warn("unable to read standard input");
      continue;
    }
    if (r == 0) {
      if (errno == error_intr) continue;
      break; /* eof */
    }
    if (r > linelen) r =linelen;
    if (timestamp && (eol == 0)) {
      taia_now(&now);
      switch (timestamp) {
      case 1:
	stamp[fmt_taia(stamp, &now)] =' ';
	stamp[26] =0;
	break;
      case 2:
	stamp[fmt_ptime(stamp, &now)] =' ';
	stamp[26] =0;
	break;
      case 3:
	stamp[fmt_ptime(stamp, &now)] =' ';
	stamp[19] =' '; stamp[20] =0;
	break;
      }
    }
    ch =buffer_peek(&data);
    for (len =0; len < r; ++len, ++ch) {
      if (*ch == '\n') {
	eol =2;
	break;
      }
      line[len] =*ch;
      
      if (repl) {
	if ((line[len] < 33) || (line[len] > 126))
	  line[len] =repl;
	else
	  for (i =0; replace[i]; ++i)
	    if (line[len] == replace[i]) {
	      line[len] =repl;
	      break;
	    }
      }
    }
    buffer_seek(&data, len);
    for (i =0; i < dirn; ++i)
      if (dir[i].fddir != -1) {
	switch(eol) {
	case 0: linestart(&dir[i], line, len); break;
	case 1: lineadd(&dir[i], line, len); break;
	case 2: lineflush(&dir[i], line, len); break;
	}
      }
    if (eol == 0) eol =1;
    if (eol == 2) { eol =0; buffer_seek(&data, 1); }
  }
  
  for (i =0; i < dirn; ++i) {
    if (dir[i].fddir != -1)
      buffer_flush(&dir[i].b);
    logdir_close(&dir[i]);
  }
  _exit(0);
}
