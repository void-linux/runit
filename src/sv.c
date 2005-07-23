#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "str.h"
#include "strerr.h"
#include "error.h"
#include "sgetopt.h"
#include "open.h"
#include "env.h"
#include "buffer.h"
#include "fmt.h"
#include "scan.h"
#include "tai.h"
#include "taia.h"

#define USAGE " [-v] [-w sec] action service ..."
#define USAGELSB " [-w sec] action"

#define VERSION "$Id$"

#define FATAL "fatal: "
#define FAIL  "fail: "
#define WARN  "warning: "
#define OK    "ok: "

/* #define VARSERVICE "./" */
#define VARSERVICE "/var/service/"
/* #define ETCRUNITRUNSV "/etc/runit/runsv/" */
#define USLEEPDELAY 250000

char *progname;
char *action;
char actch =0;
char **service;
char **servicex;
unsigned int services;
unsigned int rc =0;
unsigned int lsb;
unsigned int verbose =0;
unsigned long wait =7;
struct taia tstart, tnow, tdiff;
struct tai tstatus;

int (*act)() =0;
int (*cbk)() =0;

int curdir, fd, r;
char svstatus[20];
char sulong[FMT_ULONG];

void usage() {
  if (!lsb) strerr_die4x(100, "usage: ", progname, USAGE, "\n");
  strerr_die4x(2, "usage: ", progname, USAGELSB, "\n");
}
void done(unsigned int e) { if (curdir != -1) fchdir(curdir); _exit(e); }
void fatal(char *m1) {
  strerr_warn3(FATAL, m1, ": ", &strerr_sys);
  done(lsb ? 150 : 100); }
void fatal2(char *m1, char *m2) {
  strerr_warn4(FATAL, m1, m2, ": ", &strerr_sys);
  done(lsb ? 150 : 100);
}
void out(char *p, char *m1) {
  buffer_puts(buffer_1, p);
  buffer_puts(buffer_1, *service);
  buffer_puts(buffer_1, ": ");
  buffer_puts(buffer_1, m1);
  if (errno) {
    buffer_puts(buffer_1, ": ");
    buffer_puts(buffer_1, error_str(errno));
  }
  buffer_puts(buffer_1, "\n");
  buffer_flush(buffer_1);
}
void fail(char *m1) { ++rc; out(FAIL, m1); }
void failx(char *m1) { errno =0; fail(m1); }
void warn(char *m1) { ++rc; out(WARN, m1); }
void warnx(char *m1) { errno =0; warn(m1); }
void ok(char *m1) { errno =0; out(OK, m1); }

void outs(const char *s) { buffer_puts(buffer_1, s); }
void flush(const char *s) { outs(s); buffer_flush(buffer_1); }

int svstatus_get() {
  if ((fd =open_write("supervise/ok")) == -1) {
    if (errno == error_nodevice) {
      actch == 'x' ? ok("runsv not running") : failx("runsv not running");
      return(0);
    }
    warn("unable to open supervise/ok");
    return(-1);
  }
  close(fd);
  if ((fd =open_read("supervise/status")) == -1) {
    warn("unable to open supervise/status");
    return(-1);
  }
  r =read(fd, svstatus, 20);
  close(fd);
  switch(r) {
  case 20: break;
  case -1: warn("unable to read supervise/status"); return(-1);
  default: warnx("unable to read supervise/status: bad format"); return(-1);
  }
  return(1);
}
unsigned int svstatus_print(char *m) {
  int pid;
  int normallyup =0;
  struct stat s;
 
  if (stat("down", &s) == -1) {
    if (errno != error_noent) {
      outs(WARN); outs("unable to stat down: "); outs(error_str(errno));
      return(0);
    }
    normallyup =1;
  }
  pid =(unsigned char) svstatus[15];
  pid <<=8; pid +=(unsigned char)svstatus[14];
  pid <<=8; pid +=(unsigned char)svstatus[13];
  pid <<=8; pid +=(unsigned char)svstatus[12];
  tai_unpack(svstatus, &tstatus);
  if (pid) {
    switch (svstatus[19]) {
    case 1: outs("run: "); break;
    case 2: outs("finish: "); break;
    }
    outs(m); outs(": (pid "); sulong[fmt_ulong(sulong, pid)] =0;
    outs(sulong); outs(") ");
  }
  else {
    outs("down: "); outs(m); outs(": ");
  }
  buffer_put(buffer_1, sulong,
    fmt_ulong(sulong, tnow.sec.x < tstatus.x ? 0 : tnow.sec.x -tstatus.x));
  outs("s");
  if (pid && !normallyup) outs(", normally down");
  if (!pid && normallyup) outs(", normally up");
  if (pid && svstatus[16]) outs(", paused");
  if (!pid && (svstatus[17] == 'u')) outs(", want up");
  if (pid && (svstatus[17] == 'd')) outs(", want down");
  if (pid && svstatus[18]) outs(", got TERM");
  return(pid ? 1 : 2);
}
int status() {
  r =svstatus_get();
  switch(r) { case -1: if (lsb) done(4); case 0: return(0); }
  r =svstatus_print(*service);
  if (chdir("log") == -1) {
    if (errno != error_noent) {
      outs("; log: "); outs(WARN);
      outs("unable to change to log service directory: ");
      outs(error_str(errno));
    }
  }
  else
    if (svstatus_get()) {
      outs("; ");
      svstatus_print("log");
    }
  flush("\n");
  if (lsb) switch(r) { case 1: done(0); case 2: done(3); case 0: done(4); }
  return(r);
}
 
int check() {
  unsigned int pid;

  if (svstatus_get() <= 0) return(-1);
  pid =(unsigned char) svstatus[15];
  pid <<=8; pid +=(unsigned char)svstatus[14];
  pid <<=8; pid +=(unsigned char)svstatus[13];
  pid <<=8; pid +=(unsigned char)svstatus[12];
  switch (actch) {
  case 'u': if (!pid) return(0); break;
  case 'd': if (pid) return(0); break;
  case 't':
    tai_unpack(svstatus, &tstatus);
    if ((tstart.sec.x > tstatus.x) || ! pid || svstatus[18]) return(0);
    break;
  case 'o':
    tai_unpack(svstatus, &tstatus);
    if ((! pid && tstart.sec.x > tstatus.x) || (pid && svstatus[17] != 'd'))
      return(0);
  }
  outs(OK);
  svstatus_print(*service); flush("\n");
  return(1);
}
int check_shtdown() {
  if ((r =svstatus_get()) == -1) return(-1);
  if (r == 0) return(1);
  return(0);
}

int control() {
  if (svstatus_get() <= 0) return(-1);
  if (svstatus[17] == actch) return(0);
  if ((fd =open_write("supervise/control")) == -1) {
    if (errno != error_nodevice)
      warn("unable to open supervise/control");
    else
      actch == 'x' ? ok("runsv not running") : failx("runsv not running");
    return(-1);
  }
  r =write(fd, &actch, 1);
  close(fd);
  if (r != 1) {
    warn("unable to write to supervise/control");
    return(-1);
  }
  return(1);
}

int main(int argc, char **argv) {
  unsigned int i, done;
  char *x;

  progname =*argv;
  for (i =str_len(*argv); i; --i) if ((*argv)[i -1] == '/') break;
  *argv +=i;
  optprogname =progname =*argv;
  service =argv;
  services =1;
  lsb =(str_diff(progname, "sv"));
  if ((x =env_get("SVWAIT"))) scan_ulong(x, &wait);
  while ((i =getopt(argc, (const char* const*)argv, "w:vV")) != opteof) {
    switch(i) {
    case 'w': scan_ulong(optarg, &wait);
    case 'v': verbose =1; break;
    case 'V':
      strerr_warn1("$Id$", 0);
    case '?': usage();
    }
  }
  argv +=optind; argc -=optind;
  if (!(action =*argv++)) usage(); --argc;
  if (!lsb) {
    service =argv;
    services =argc;
  }
  if (! *service) usage();

  taia_now(&tnow); tstart =tnow;
  if ((curdir =open_read(".")) == -1)
    fatal("unable to open current directory");

  act =&control;
  if (verbose) cbk =&check;
  switch (*action) {
  case 'x': case 'e': action ="x";
  case 'u': case 'd': case 'o': case 't': case 'p': case 'c': case 'h':
  case 'a': case 'i': case 'k': case 'q': case '1': case '2':
    actch =*action; break;
  case 's':
    if (!str_diff(action, "shutdown")) {
      actch ='x'; cbk =&check_shtdown; break;
    }
    if (!str_diff(action, "start")) { actch ='u'; cbk =&check; break; }
    if (!str_diff(action, "stop")) { actch ='d'; cbk =&check; break; }
    if (lsb && str_diff(action, "status")) usage();
    act =&status; cbk =0; break;
  case 'r':
    if (!str_diff(action, "restart")) { actch ='t'; cbk =&check; break; }
    usage();
  case 'f':
    if (!str_diff(action, "force-reload")) { actch ='t'; cbk =&check; break; }
    if (!str_diff(action, "force-shutdown"))
      { actch ='t'; cbk =&check; break; }
  default:
    usage();
  }

  servicex =service;
  for (i =0; i < services; ++i) {
    if ((**service != '/') && (**service != '.')) {
      if ((chdir(VARSERVICE) == -1) || (chdir(*service) == -1)) {
        fail("unable to change to service directory");
        *service =0;
      }
    }
    else
      if (chdir(*service) == -1) {
        fail("unable to change to service directory");
        *service =0;
      }
    if (*service) if (act() == -1) *service =0;
    if (fchdir(curdir) == -1) fatal("unable to change to original directory");
    service++;
  }

  if (*cbk)
    for (;;) {
      taia_sub(&tdiff, &tnow, &tstart);
      service =servicex; done =1;
      for (i =0; i < services; ++i, ++service) {
        if (! *service) continue;
        if ((**service != '/') && (**service != '.')) {
          if ((chdir(VARSERVICE) == -1) || (chdir(*service) == -1)) {
            fail("unable to change to service directory");
            *service =0;
          }
        }
        else
          if (chdir(*service) == -1) {
            fail("unable to change to service directory");
            *service =0;
          }
        if (taia_approx(&tdiff) > wait) {
          outs("timeout "); sulong[fmt_ulong(sulong, wait)] =0; outs(sulong);
          outs("s: ");
          if (svstatus_get() > 0) { svstatus_print(*service); ++rc; }
          flush("\n");
          *service =0;
        }
        if (*service) { if (cbk() != 0) *service =0; else done =0; }
        if (fchdir(curdir) == -1)
          fatal("unable to change to original directory");
      }
      if (done) break;
      usleep(USLEEPDELAY);
      taia_now(&tnow);
    }
  return(rc > 99 ? 99 : rc);
}
