DESTDIR=

CC=./support/compile
LD=./support/load

CFLAGS=-Isupport

all: runit runit-init svwaitup svwaitdown

runit: djb-publicdomain runit.o
	$(LD) runit support/unix.a support/byte.a -static

runit-init: djb-publicdomain runit-init.o
	$(LD) runit-init support/unix.a support/byte.a -static

svwaitup: djb-publicdomain svwaitup.o
	$(LD) svwaitup support/unix.a support/byte.a support/time.a

svwaitdown: djb-publicdomain svwaitdown.o
	$(LD) svwaitdown support/unix.a support/byte.a support/time.a

djb-publicdomain:
	( cd support ; make )

clean:
	find . -name \*~ -exec rm -f {} \;
	find . -name .??*~ -exec rm -f {} \;
	find . -name \#?* -exec rm -f {} \;
	if test -h support; \
		then ( cd support ; $(MAKE) clean ); fi
	rm -f `cat TARGETS`
