#include <utmp.h>

int main (int argc,  char **argv) {
  argv++;
  logwtmp(*argv, 0, 0);
  return(0);
}
