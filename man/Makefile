-include ../config.mk

MANPAGES = chpst.8 runit-init.8 runit.8 svlogd.8 runsvdir.8
MANPAGES_IN = runsvchdir.8 sv.8 runsv.8 utmpset.8

.PHONY: all install clean

all: $(MANPAGES) $(MANPAGES_IN)

$(MANPAGES_IN):
	$(SED) -e "s|@@SERVICEDIR@@|${SERVICEDIR}|g" $@.in > $@

install: all
	install -d $(DESTDIR)$(MANDIR)/man8
	for page in $(MANPAGES) $(MANPAGES_IN); do \
		install -m644 $$page $(DESTDIR)$(MANDIR)/man8; \
	done

clean:
	rm -f $(MANPAGES_IN)
