#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utmp.h>
#include "strerr.h"
#include "sgetopt.h"
#include "seek.h"
#include "str.h"
#include "open.h"
#include "byte.h"

#define USAGE " [-w] line"
#define WARNING "utmpset: warning: "

const char *progname;

void usage(void) {
  strerr_die4x(1, "usage: ", progname, USAGE, "\n");
}

int utmp_logout(const char *line) {
  int fd;
  struct utmp ut;
  int ok =-1;

  if ((fd =open(_PATH_UTMP, O_RDWR, 0)) < 0)
    return(-1);
  while (read(fd, &ut, sizeof(struct utmp)) == sizeof(struct utmp)) {
    if (!ut.ut_name[0] || (str_diff(ut.ut_line, line) != 0))
      continue;
    memset(ut.ut_name, 0, UT_NAMESIZE);
    memset(ut.ut_host, 0, UT_HOSTSIZE);
    if (time(&ut.ut_time) == -1) break;
    if (lseek(fd, -(off_t)sizeof(struct utmp), SEEK_CUR) == -1) break;
    if (write(fd, &ut, sizeof(struct utmp)) != sizeof(struct utmp)) break;
    ok =1;
    break;
  }
  close(fd);
  return(ok);
}
int wtmp_logout(const char *line) {
  int fd;
  int len;
  struct stat st;
  struct utmp ut;

  if ((fd = open_append(_PATH_WTMP)) == -1)
    return(-1);
  if (fstat(fd, &st) == -1) {
    close(fd);
    return(-1);
  }
  memset(&ut, 0, sizeof(struct utmp));
  if ((len =str_len(line)) > UT_LINESIZE)
    len =UT_LINESIZE -1;
  byte_copy(ut.ut_line, len, line);
  if (time(&ut.ut_time) == -1) {
    close(fd);
    return(-1);
  }
  if (write(fd, &ut, sizeof(struct utmp)) != sizeof(struct utmp)) {
    ftruncate(fd, st.st_size);
    close(fd);
    return(-1);
  }
  close(fd);
  return(1);
}

int main (int argc, const char * const *argv, const char * const *envp) {
  int opt;
  int wtmp =0;

  progname =*argv;

  while ((opt =getopt(argc, argv, "wV")) != opteof) {
    switch(opt) {
    case 'w':
      wtmp =1;
      break;
    case 'V':
      strerr_warn1("$Id$", 0);
    case '?':
      usage();
    }
  }
  argv +=optind;

  if (! argv || ! *argv) usage();
  if (utmp_logout(*argv) == -1)
    strerr_die4sys(111, WARNING, "unable to utmp logout line ", *argv, ": ");
  if (wtmp) {
    if (wtmp_logout(*argv) == -1)
      strerr_die4sys(111, WARNING, "unable to wtmp logout line ", *argv, ": ");
  }
  exit(0);
}
