-include config.mk

SUBDIRS = src man completions

.PHONY: all install uninstall check clean

all:
	@if test ! -e config.mk; then \
		echo "You didn't run ./configure ... exiting."; \
		exit 1; \
	fi
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

install: all
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir install || exit 1; \
	done

uninstall:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir uninstall || exit 1; \
	done

check: all
	$(MAKE) -C src check

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean || exit 1; \
	done
	-rm -f result* _ccflag.{,c,err}
