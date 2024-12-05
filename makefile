CFLAGS = -std=c99 -Wall -Wextra

all: logwrap

logwrap: logwrap.o

logwrap.o: logwrap.c

clean:
	$(RM) logwrap.o

distclean: clean
	$(RM) logwrap

.PHONY: all clean distclean
