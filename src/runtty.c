#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
              
#include "open.h"
#include "pathexec.h"
#include "strerr.h"
#include "ndelay.h"

#define FATAL "runtty: fatal: "

const char *progname;

int main (int argc, const char * const *argv, const char * const *envp) {
  const char *dev;
  const char * const *prog;
  int ttyfd;

  progname =*argv++;

  dev =*argv++;
  prog =argv++;

  setsid();
  
  /* console */
  if ((ttyfd =open_write(dev)) != -1) {
    //    if (ioctl(ttyfd, TIOCSCTTY, (char *)NULL) != -1) {
      dup2(ttyfd, 1); dup2(ttyfd, 2);
      if (ttyfd > 2)
        close(ttyfd);
      //    }
      //    else {
      //      strerr_die2sys(111, FATAL, "ioctl(): ");
      //    }
  }
  else {
    strerr_die4sys(111, FATAL, "unable to open ", dev, ": ");
  }
  if ((ttyfd =open_read(dev)) != -1) {
    ndelay_off(ttyfd);
    dup2(ttyfd, 0);
    if (ttyfd > 2)
      close(ttyfd);
  }
  else {
    strerr_die4sys(111, FATAL, "unable to open ", dev, ": ");
  }

  pathexec_run(*prog, prog, envp);
  strerr_die3sys(111, FATAL, "unable to run ", *prog);
  return(0);
}
