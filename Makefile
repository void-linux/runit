DESTDIR=

PACKAGE=runit-0.5.4
DIRS=doc man etc package src
MANPAGES=runit.8 runit-init.8 runsvdir.8 runsv.8 svwaitdown.8 svwaitup.8 \
utmpset.8

all: clean .manpages $(PACKAGE).tar.gz

.manpages:
	for i in $(MANPAGES); do \
	  rman -S -f html -r '' < man/$$i | \
	  sed -e 's}NAME="sect\([0-9]*\)" HREF="#toc[0-9]*">\(.*\)}NAME="sect\1">\2}g ; \
	  s}<A HREF="#toc">Table of Contents</A>}<a href="http://smarden.org/pape/">G. Pape</a><br><A HREF="index.html">runit</A><hr>}g ; \
	  s}<!--.*-->}}g' \
	  > doc/$$i.html ; \
	done ; \
	touch .manpages

$(PACKAGE).tar.gz:
	rm -rf TEMP
	mkdir -p TEMP/admin/$(PACKAGE)
	make -C src clean
	cp -a $(DIRS) TEMP/admin/$(PACKAGE)/
	ln -sf ../etc/debian TEMP/admin/$(PACKAGE)/doc/
	chmod -R g-ws TEMP/admin
	chmod +t TEMP/admin
	find TEMP -exec touch {} \;
	su -c 'chown -R root:root TEMP/admin ; \
		( cd TEMP ; tar cpfz ../$(PACKAGE).tar.gz admin --exclude CVS ) ; \
		rm -rf TEMP'

clean:
	find . -name \*~ -exec rm -f {} \;
	find . -name .??*~ -exec rm -f {} \;
	find . -name \#?* -exec rm -f {} \;

cleaner: clean
	rm -f $(PACKAGE).tar.gz
	for i in $(MANPAGES); do rm -f doc/`basename $$i`.html; done
	rm -f .manpages
