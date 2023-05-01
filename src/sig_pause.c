/* Public domain. */

#include <signal.h>
#include "sig.h"

void sig_pause(void)
{
#ifdef HASSIGPROCMASK
  sigset_t ss;
  sigemptyset(&ss);
  sigsuspend(&ss);
#else
  sigpause(0);
#endif
}
