-include ../config.mk

COMPLETIONS = sv.bash sv.zsh

.PHONY: all install clean

all: $(COMPLETIONS)

$(COMPLETIONS):
	$(SED) -e "s|@@SERVICEDIR@@|${SERVICEDIR}|g" $@.in > $@

install: all
	install -d $(DESTDIR)$(SHAREDIR)/bash-completion/completions
	install -m644 sv.bash $(DESTDIR)$(SHAREDIR)/bash-completion/completions/sv
	install -d $(DESTDIR)$(SHAREDIR)/zsh/site-functions
	install -m644 sv.zsh $(DESTDIR)$(SHAREDIR)/zsh/site-functions/_sv

clean:
	rm -f $(COMPLETIONS)
