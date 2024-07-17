CFLAGS=$(shell cat ../../build-cflags.txt)

BUILDDIR=$(CURDIR)/build

.PHONY: all
all: corodemo

corodemo: corodemo.c coro.c | $(BUILDDIR)
	gcc coro.c corodemo.c -O0 -g -DCORO_DEBUG -o $(BUILDDIR)/corodemo

$(BUILDDIR):
	mkdir $(BUILDDIR)

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
