DESTDIR=

install:
	for i in 2 3 4; do install -d $(DESTDIR)/etc/runit/getty-$$i; done
	for i in 2 3 4; do \
	  install -m0755 getty-$$i/run $(DESTDIR)/etc/runit/getty-$$i/run; \
	  install -m0755 getty-$$i/finish $(DESTDIR)/etc/runit/getty-$$i/finish; \
	done
