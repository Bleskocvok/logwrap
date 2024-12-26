CFLAGS = -std=c99 -Wall -Wextra

ifdef DEBUG
	CFLAGS += -g
endif

all: logwrap

logwrap: logwrap.o

logwrap.o: logwrap.c

test: logwrap
	$(MAKE) -C test/ run_all -j

clean:
	$(MAKE) -C test/ clean
	$(RM) logwrap.o

distclean: clean
	$(MAKE) -C test/ distclean
	$(RM) logwrap

.PHONY: all clean distclean test
