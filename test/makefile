CFLAGS = -std=c99 -Wall -Wextra

all: run_test test_server test_cmd test_exit test_signal

%: %.o

clean:
	$(RM) *.o

distclean: clean
	$(RM) run_test test_server test_cmd test_exit test_signal

.PHONY: all clean distclean
