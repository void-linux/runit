/*
  in /package/admin/daemontools/compile/ :
  gcc -Wall -c svwaitup.c && gcc -o svwaitup svwaitup.o unix.a byte.a time.a

  Gerrit Pape <pape@smarden.org>
*/

#include <unistd.h>
#include "strerr.h"
#include "error.h"
#include "sgetopt.h"
#include "scan.h"
#include "open.h"
#include "tai.h"
#include "buffer.h"

#define FATAL "svwaitup: fatal: "
#define WARNING "svwaitup: warning: "
#define INFO "svwaitup: "
#define USAGE " [ -v ] [ -s 2..600 ] service ..."

const char *progname;
unsigned long sec;
unsigned int rc =0;
const char * const *dir;

void usage () {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

void warn (const char *s1, const char *s2, struct strerr *e) {
  dir++;
  rc++;
  strerr_warn3(WARNING, s1, s2, e);
}

int main (int argc, const char * const *argv) {
  int opt;
  unsigned long sec =2;
  int verbose =0;
  char status[18];
  int fd;
  int is;
  int r;
  unsigned long pid;
  struct tai when;
  struct tai now;
  
  progname =*argv;
  
  while ((opt =getopt(argc, argv, "s:vV")) != opteof) {
    switch(opt) {
    case 's': 
      scan_ulong(optarg, &sec);
      if ((sec < 2) || (sec > 600)) {
	usage();
      }
      break;
    case 'v':
      verbose =1;
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
  
  dir =argv;

  while (*dir) {
    if (chdir(*dir) == -1) {
      warn(*dir, ": unable to change directory: ", &strerr_sys);
      continue;
    }
    
    fd = open_write("supervise/ok");
    if (fd == -1) {
      if (errno == error_nodevice)
	warn(*dir, ": supervise not running.", 0);
      else
	warn(*dir, ": unable to open supervise/ok: ", &strerr_sys);
      continue;
    }
    close(fd);
    
    fd = open_read("supervise/status");
    if (fd == -1) {
      warn(*dir, "unable to open supervise/status: ", &strerr_sys);
      continue;
    }
    r = buffer_unixread(fd, status, sizeof status);
    close(fd);
    if (r < sizeof status) {
      if (r == -1)
	warn(*dir, "unable to read supervise/status: ", &strerr_sys);
      else
	warn(*dir, ": unable to read supervise/status: bad format.", 0);
      continue;
    }
    
    pid = (unsigned char) status[15];
    pid <<= 8; pid += (unsigned char) status[14];
    pid <<= 8; pid += (unsigned char) status[13];
    pid <<= 8; pid += (unsigned char) status[12];
    if (! pid) {
      warn(*dir, ": is down.", 0);
      continue;
    }
    
    tai_unpack(status, &when);
    tai_now(&now);
    if (tai_less(&now, &when)) when =now;
    tai_sub(&when, &now, &when);
    is =tai_approx(&when);
    
    if (is >= sec) {
      /* ok */
      if (verbose) strerr_warn3(INFO, *dir, ": is up.", 0);
      dir++;
      continue;
    }
    sleep(sec -is);
  }
  if (rc > 100) rc =100;
  exit(rc);
}
