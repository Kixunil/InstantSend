PREFIX=/usr

all: main plugins

main:
	$(MAKE) --directory src

plugins:
	$(MAKE) --directory plugins


clean:
	$(MAKE) --directory src clean
	$(MAKE) --directory plugins clean

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/share/instantsend/data
	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	mkdir -p $(DESTDIR)$(PREFIX)/lib/instantsend/plugins
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)/etc/instantsend/
	cp src/server $(DESTDIR)$(PREFIX)/bin/instsendd
	cp src/client $(DESTDIR)$(PREFIX)/bin/isend # Short name for better user experience
	cp src/widget-gtk $(DESTDIR)$(PREFIX)/bin/instsend-dialog-gtk
	cp src/configtool $(DESTDIR)$(PREFIX)/bin/instsend-config
	cp scripts/isend-gtk.sh $(DESTDIR)$(PREFIX)/bin/isend-gtk
	cp plugins/*.so $(DESTDIR)$(PREFIX)/lib/instantsend/plugins
	cp data/icon_32.png $(DESTDIR)$(PREFIX)/share/instantsend/data/
	cp data/dialog-gtk.desktop $(DESTDIR)$(PREFIX)/share/applications/instsend-dialog-gtk.desktop
	chmod 755 $(DESTDIR)$(PREFIX)/bin/instsendd $(DESTDIR)$(PREFIX)/bin/isend $(DESTDIR)$(PREFIX)/bin/instsend-dialog-gtk $(DESTDIR)$(PREFIX)/bin/instsend-config $(DESTDIR)$(PREFIX)/bin/isend-gtk

uninstall:
	rm -rf $(DESTDIR)$(PREFIX)/share/instantsend  # Remove data
	rm -rf $(DESTDIR)$(PREFIX)/lib/instantsend    # Remove plugins
	rm -rf /etc/instantsend             # Remove configuration
	rm -f $(DESTDIR)$(PREFIX)/bin/instsendd       # Remove server
	rm -f $(DESTDIR)$(PREFIX)/bin/isend           # Remove client
	rm -f $(DESTDIR)$(PREFIX)/bin/instsend-dialog # Remove dialog
	rm -f $(DESTDIR)$(PREFIX)/bin/instsend-config # Remove configuration tool

.PHONY: main plugins all install
