/* Public domain. */

#include <signal.h>
#include "sig.h"

const int sig_alarm = SIGALRM;
const int sig_child = SIGCHLD;
const int sig_cont = SIGCONT;
const int sig_hangup = SIGHUP;
const int sig_int = SIGINT;
const int sig_kill = SIGKILL;
const int sig_pipe = SIGPIPE;
const int sig_term = SIGTERM;

void (*sig_defaulthandler)() = SIG_DFL;
void (*sig_ignorehandler)() = SIG_IGN;
