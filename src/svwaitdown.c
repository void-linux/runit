#include <unistd.h>
#include "strerr.h"
#include "error.h"
#include "sgetopt.h"
#include "scan.h"
#include "open.h"
#include "tai.h"
#include "buffer.h"

#define FATAL "svwaitdown: fatal: "
#define WARNING "svwaitdown: warning: "
#define INFO "svwaitdown: "
#define USAGE " [ -v ] [ -t 2..6000 ] service ..."

const char *progname;
const char * const *dir;
unsigned int rc =0;

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
  unsigned long sec =600;
  int verbose =0;
  int doexit =0;
  int dokill =0;
  int fd;
  char status[18];
  int r;
  unsigned long pid;
  struct tai start;
  struct tai now;
  
  progname =*argv;
  
  while ((opt =getopt(argc, argv, "t:xkvV")) != opteof) {
    switch(opt) {
    case 't':
      scan_ulong(optarg, &sec);
      if ((sec < 2) || (sec > 6000)) {
	usage();
      }
      break;
    case 'x':
      doexit =1;
      break;
    case 'k':
      dokill =1;
      break;
    case 'v':
      verbose =1;
      break;
    case 'V':
      strerr_warn1("$Id$", 0);
    case '?':
      usage();
    }
  }
  argv +=optind;
  if (! argv || ! *argv) {
    usage();
  }

  dir =argv;
  tai_now(&start);
  while (*dir) {
    if (*dir[0] != '/') {
      warn(*dir, ": service directory must start with a slash.", 0);
      continue;
    }
    if (chdir(*dir) == -1) {
      warn(*dir, ": unable to change directory: ", &strerr_sys);
      continue;
    }
    
    fd = open_write("supervise/ok");
    if (fd == -1) {
      if (errno == error_nodevice) {
	if (verbose) strerr_warn3(INFO, *dir, ": supervise not running.", 0);
	dir++;
      }
      else
	warn(*dir, ": unable to open supervise/ok: ", &strerr_sys);
      continue;
    }
    close(fd);

    if (! doexit) {
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
	/* ok, service is down */
	if (verbose) strerr_warn3(INFO, *dir, ": down.", 0);
	dir++;
	continue;
      }
    }
    tai_now(&now);
    tai_sub(&now, &now, &start);
    if (tai_approx(&now) >= sec) {
      /* timeout */
      if (verbose) strerr_warn2(INFO, "timeout.", 0);
      if (dokill) {
	dir =argv;
	while (*dir) {
	  if (chdir(*dir) == -1) {
	    warn(*dir, ": unable to change directory: ", &strerr_sys);
	    continue;
	  }
	  if ((fd =open_write("supervise/control")) == -1) {
	    if (errno == error_nodevice) {
	      if (verbose)
		strerr_warn3(INFO, *dir,
			     ": supervise not running.", 0);
	      dir++;
	    }
	    else
	      warn(*argv, ": unable to open supervise/control: ", &strerr_sys);
	    continue;
	  }
	  if (write(fd, "k", 1) != 1)
	    warn(*argv,
		 ": unable to write to supervise/control: ", &strerr_sys);
	  else
	    strerr_warn3(INFO, *dir, ": killed.", 0);
	  close(fd);
	  dir++;
	}
      }
      exit(111);
    }

    sleep(1);
  }
  if (rc > 100) rc =100;
  exit(rc);
}
