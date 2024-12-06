CFLAGS = -std=c99 -Wall -Wextra

all: run_test test_server test_cmd

%: %.o

clean:
	$(RM) *.o

distclean: clean
	$(RM) run_test test_server test_cmd

.PHONY: all clean distclean
