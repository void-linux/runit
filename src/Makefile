DESTDIR=

CC=gcc

CFLAGS=-Wall -O3 -Isupport
LDFLAGS=-Wall -O3 -s

all: runit runit-init svwaitup svwaitdown

set-tcp-env: djb-publicdomain set-tcp-env.o
	$(CC) $(LDFLAGS) -o set-tcp-env \
		set-tcp-env.o support/unix.a support/byte.a

runit: djb-publicdomain runit.o
	$(CC) $(LDFLAGS) -o runit \
		runit.o support/unix.a support/byte.a

runit-init:	djb-publicdomain init.o
	$(CC) $(LDFLAGS) -o runit-init \
		init.o support/unix.a support/byte.a

svwaitup:	djb-publicdomain svwaitup.o
	$(CC) $(LDFLAGS) -o svwaitup \
		svwaitup.o support/unix.a support/byte.a support/time.a

svwaitdown:	djb-publicdomain svwaitdown.o
	$(CC) $(LDFLAGS) -o svwaitdown \
		svwaitdown.o support/unix.a support/byte.a support/time.a

djb-publicdomain:
	( cd support ; make )

clean:
	find . -name \*~ -exec rm -f {} \;
	find . -name .??*~ -exec rm -f {} \;
	find . -name \#?* -exec rm -f {} \;
	if test -h support; \
		then ( cd support ; $(MAKE) clean ); fi
	rm -f `cat TARGETS`
