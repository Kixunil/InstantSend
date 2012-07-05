main:
	$(MAKE) --directory src

plugins:
	$(MAKE) --directory plugins

all: main, plugins
