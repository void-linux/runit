#include <sys/types.h>
#include <pwd.h>
#include "prot.h"
#include "strerr.h"
#include "pathexec.h"

#define USAGE " account child"
#define FATAL "setuidgid: fatal: "

const char *progname;

void fatal(char *m) { strerr_die3sys(111, FATAL, m, ": "); }
void usage() { strerr_die4x(100, "usage: ", progname, USAGE, "\n"); }

int main(int argc, const char *const *argv, const char *const *envp) {
  const char *account;
  struct passwd *pw;

  progname =argv[0];

  if (! (account =*++argv)) usage();
  if (! *++argv) usage();

  if (! (pw =getpwnam(account)))
    strerr_die3x(111, FATAL, "unknown account ", account);

  if (prot_gid(pw->pw_gid) == -1) fatal("unable to setgid");
  if (prot_uid(pw->pw_uid) == -1) fatal("unable to setuid");

  pathexec_run(*argv, argv, envp);
  strerr_die4sys(111, FATAL, "unable to run ", *argv, ": ");
  return(1);
}
