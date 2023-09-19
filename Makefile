SHELL=/bin/bash

.PHONY: all dns http https others ping smtp tcp udp

all:
	@for a in $$(ls); do \
	if [ -d $$a ]; then \
		echo "processing folder $$a"; \
		$(MAKE) -C $$a; \
	fi; \
	done;
	@echo "Done!"
	
clean:
	@for a in $$(ls); do \
	if [ -d $$a ]; then \
		echo "processing folder $$a"; \
		$(MAKE) -C $$a clean; \
	fi; \
	done;
	@echo "Done!"

dns:
	$(MAKE) -C $(PWD)/dns
	@echo "Done!"
http:
	$(MAKE) -C $(PWD)/http
	@echo "Done!"
https:
	$(MAKE) -C $(PWD)/https
	@echo "Done!"
others:
	$(MAKE) -C $(PWD)/others
	@echo "Done!"
ping:
	$(MAKE) -C $(PWD)/ping
	@echo "Done!"
smtp:
	$(MAKE) -C $(PWD)/smtp
	@echo "Done!"
tcp:
	$(MAKE) -C $(PWD)/tcp
	@echo "Done!"
udp:
	$(MAKE) -C $(PWD)/udp
	@echo "Done!"
