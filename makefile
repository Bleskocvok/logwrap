CFLAGS = -std=c99 -Wall -Wextra

all: logwrap

logwrap: logwrap.o

logwrap.o: logwrap.c

test: logwrap
	$(MAKE) -C test/
	cd test && time -p ./run_test

clean:
	$(MAKE) -C test/ clean
	$(RM) logwrap.o

distclean: clean
	$(MAKE) -C test/ distclean
	$(RM) logwrap

.PHONY: all clean distclean test
