/* Public domain. */

#ifndef BYTE_H
#define BYTE_H

extern unsigned int byte_chr(char *s, register unsigned int n, int c);
extern unsigned int byte_rchr(char *s, register unsigned int n, int c);
extern void byte_copy(register char *to, register unsigned int n, register char *from);
extern void byte_copyr(register char *to, register unsigned int n, register char *from);
extern int byte_diff(register char *s, register unsigned int n, register char *t);

#define byte_equal(s,n,t) (!byte_diff((s),(n),(t)))

#endif
