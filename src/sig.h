/* Public domain. */

#ifndef SIG_H
#define SIG_H

extern const int sig_alarm;
extern const int sig_child;
extern const int sig_cont;
extern const int sig_hangup;
extern const int sig_int;
extern const int sig_kill;
extern const int sig_pipe;
extern const int sig_term;

extern void (*sig_defaulthandler)();
extern void (*sig_ignorehandler)();

extern void sig_catch(int,void (*)());
#define sig_ignore(s) (sig_catch((s),sig_ignorehandler))
#define sig_uncatch(s) (sig_catch((s),sig_defaulthandler))

extern void sig_block(int);
extern void sig_unblock(int);
extern void sig_blocknone(void);
extern void sig_pause(void);

extern void sig_dfl(int);

#endif
