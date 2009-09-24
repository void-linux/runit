DESTDIR=

PACKAGE=runit-2.1.0
DIRS=doc man etc package src
MANPAGES=runit.8 runit-init.8 runsvdir.8 runsv.8 sv.8 utmpset.8 \
  runsvchdir.8 svlogd.8 chpst.8

all: clean .manpages $(PACKAGE).tar.gz

.manpages:
	for i in $(MANPAGES); do \
	  rman -S -f html -r '' < man/$$i | \
	  sed -e "s}name='sect\([0-9]*\)' href='#toc[0-9]*'>\(.*\)}name='sect\1'>\2}g ; \
	  s}<a href='#toc'>Table of Contents</a>}<a href='http://smarden.org/pape/'>G. Pape</a><br><a href='index.html'>runit</A><hr>}g ; \
	  s}<!--.*-->}}g" \
	  > doc/$$i.html ; \
	done ; \
	echo 'fix up html manually...'
	echo 'patch -p0 <manpagehtml.diff && exit'
	sh
	find . -name '*.orig' -exec rm -f {} \;
	touch .manpages

$(PACKAGE).tar.gz:
	rm -rf TEMP
	mkdir -p TEMP/admin/$(PACKAGE)
	make -C src clean
	cp -a $(DIRS) TEMP/admin/$(PACKAGE)/
	ln -sf ../etc/debian TEMP/admin/$(PACKAGE)/doc/
	for i in TEMP/admin/$(PACKAGE)/etc/*; do \
	  test -d $$i && ln -s ../2 $$i/2; \
	done
	chmod -R g-ws TEMP/admin
	chmod +t TEMP/admin
	find TEMP -exec touch {} \;
	su -c '\
	  chown -R root:root TEMP/admin ; \
	  (cd TEMP && tar --exclude CVS -cpzf ../$(PACKAGE).tar.gz admin); \
	  rm -rf TEMP'

clean:
	find . -name \*~ -exec rm -f {} \;
	find . -name .??*~ -exec rm -f {} \;
	find . -name \#?* -exec rm -f {} \;

cleaner: clean
	rm -f $(PACKAGE).tar.gz
	for i in $(MANPAGES); do rm -f doc/`basename $$i`.html; done
	rm -f .manpages
