prefix=/home/martin/instantsend
exec_prefix=${prefix}
datarootdir=${prefix}/share
LOCALEDIR=${datarootdir}/locale

GETTEXT_DOMAINNAME=instantsend-dialog-gtk

POFILES=$(wildcard *.po)
MOFILES=$(POFILES:.po=.mo)
LANGS=$(subst .po,,$(POFILES))

all: $(MOFILES)

check:

clean:
	rm -f *.mo

distclean: clean
	rm -f Makefile

%.mo: %.po
	msgfmt -o $@ $<

install-mo-%: %.mo
	install -D -m 644 $< $(DESTDIR)$(LOCALEDIR)/$(subst .mo,,$<)/LC_MESSAGES/$(GETTEXT_DOMAINNAME).mo

install: $(patsubst %,install-mo-%,$(LANGS))

.PHONY: all check clean distclean install-mo-% install
