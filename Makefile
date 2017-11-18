.POSIX:
.DELETE_ON_ERROR:

# This Makefile is for people who just type make whenever
# they see a Makefile somewhere... :-)
#
# Reading the README is a better way

.PHONY: top
top: msg all

.PHONY: msg
msg:	
	@echo
	@echo Aha, you are one of these persons that types make
	@echo whenever they see a Makefile somewhere....
	@echo

.PHONY: all
all:
	@cd src; $(MAKE) DEBUG=$(DEBUG) OPSYS=$(OPSYS) CC=$(CC)

.PHONY: install
install:
	cd bin ; $(MAKE) install

.PHONY: clean
clean:
	cd src; $(MAKE) clean

.PHONY: tar
tar: src
	cd src; $(MAKE) almostclean
	-cd bin; strip opencomal opencomalrun
	tools/gentar