/* Public domain. */

#ifndef DIRENTRY_H
#define DIRENTRY_H

#include <sys/types.h>

#ifdef HASDIRENT

#include <dirent.h>
#define direntry struct dirent

#else

#include <sys/dir.h>
#define direntry struct direct

#endif

#endif
