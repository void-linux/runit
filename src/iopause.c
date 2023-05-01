/* Public domain. */

#include "taia.h"
#include "iopause.h"

void iopause(iopause_fd *x,unsigned int len,struct taia *deadline,struct taia *stamp)
{
  struct taia t;
  int millisecs;
  double d;
  int i;

  if (taia_less(deadline,stamp))
    millisecs = 0;
  else {
    t = *stamp;
    taia_sub(&t,deadline,&t);
    d = taia_approx(&t);
    if (d > 1000.0) d = 1000.0;
    millisecs = d * 1000.0 + 20.0;
  }

  for (i = 0;i < len;++i)
    x[i].revents = 0;

  poll(x,len,millisecs);
  /* XXX: some kernels apparently need x[0] even if len is 0 */
  /* XXX: how to handle EAGAIN? are kernels really this dumb? */
  /* XXX: how to handle EINVAL? when exactly can this happen? */

}
