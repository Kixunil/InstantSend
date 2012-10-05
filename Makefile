all: main plugins

main:
	$(MAKE) --directory src

plugins:
	$(MAKE) --directory plugins


clean:
	$(MAKE) --directory src clean
	$(MAKE) --directory plugins clean

.PHONY: main plugins all
