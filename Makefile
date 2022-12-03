obj-m = xtsproxy.o

KVER ?= $(shell uname -r)
override KDIR := /lib/modules/$(KVER)/build
override PWD := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
override MAKE := $(MAKE) -C $(KDIR) M=$(PWD) V=1

all: modules

modules: clean
	$(MAKE) modules
.PHONY: modules

clean:
	$(MAKE) clean
.PHONY: clean
