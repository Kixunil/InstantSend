
all: caja nautilus

check: check-caja check-nautilus

clean: clean-caja clean-nautilus

distclean: distclean-caja distclean-nautilus

install: install-caja install-nautilus

caja:
	$(MAKE) -C caja

nautilus:
	$(MAKE) -C nautilus

check-caja:
	$(MAKE) -C caja check

check-nautilus:
	$(MAKE) -C nautilus check

clean-caja:
	$(MAKE) -C caja clean

clean-nautilus:
	$(MAKE) -C nautilus clean

distclean: distclean-caja distclean-nautilus

distclean-caja:
	$(MAKE) -C caja distclean

distclean-nautilus:
	$(MAKE) -C nautilus distclean

install-caja:
	$(MAKE) -C caja install

install-nautilus:
	$(MAKE) -C nautilus install

.PHONY: all check clean distclean install caja nautilus check-caja check-nautilus clean-caja clean-nautilus distclean-caja distclean-nautilus install-caja install-nautilus
