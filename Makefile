PREFIX=/usr/local

all: main plugins

main:
	$(MAKE) --directory src

plugins:
	$(MAKE) --directory plugins


clean:
	$(MAKE) --directory src clean
	$(MAKE) --directory plugins clean

install: all
	mkdir -p $(PREFIX)/share/instantsend/data
	mkdir -p $(PREFIX)/lib/instantsend/plugins
	mkdir -p /etc/instantsend/
	cp src/server $(PREFIX)/bin/instsendd
	cp src/client $(PREFIX)/bin/isend # Short name for better user experience
	cp src/widget-gtk $(PREFIX)/bin/instsend-dialog
	cp src/configtool $(PREFIX)/bin/instsend-config
	cp plugins/*.so $(PREFIX)/lib/instantsend/plugins
	cp data/* $(PREFIX)/share/instantsend/data/

uninstall:
	rm -rf $(PREFIX)/share/instantsend  # Remove data
	rm -rf $(PREFIX)/lib/instantsend    # Remove plugins
	rm -rf /etc/instantsend             # Remove configuration
	rm -f $(PREFIX)/bin/instsendd       # Remove server
	rm -f $(PREFIX)/bin/isend           # Remove client
	rm -f $(PREFIX)/bin/instsend-dialog # Remove dialog
	rm -f $(PREFIX)/bin/instsend-config # Remove configuration tool

.PHONY: main plugins all install
