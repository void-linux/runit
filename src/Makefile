DESTDIR=

CC=./support/compile
LOAD=./support/load

CC=gcc

CFLAGS=-Isupport

all: runit runit-init svwaitup svwaitdown

runit: djb-publicdomain runit.o
	$(LOAD) runit support/unix.a support/byte.a

runit-init: djb-publicdomain runit-init.o
	$(LOAD) runit-init support/unix.a support/byte.a

svwaitup: djb-publicdomain svwaitup.o
	$(LOAD) svwaitup support/unix.a support/byte.a support/time.a

svwaitdown: djb-publicdomain svwaitdown.o
	$(LOAD) svwaitdown support/unix.a support/byte.a support/time.a

djb-publicdomain:
	( cd support ; make )

clean:
	find . -name \*~ -exec rm -f {} \;
	find . -name .??*~ -exec rm -f {} \;
	find . -name \#?* -exec rm -f {} \;
	if test -h support; \
		then ( cd support ; $(MAKE) clean ); fi
	rm -f `cat TARGETS`
