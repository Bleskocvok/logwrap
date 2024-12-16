CFLAGS = -std=c99 -Wall -Wextra

UTILS = utils/server utils/cmd utils/exit utils/signal
TESTS = test_quit test_simple test_advanced test_advanced_newline test_timed test_timed_detach
RUN_TESTS = $(TESTS:%=run.%)

all: run_all

run_test: utils/utils.o

test_quit:             utils/utils.c
test_simple:           utils/utils.c
test_advanced:         utils/utils.c utils/test_advanced.c
test_advanced_newline: utils/utils.c utils/test_advanced.c
test_timed:            utils/utils.c utils/test_timed.c
test_timed_detach:     utils/utils.c utils/test_timed.c

server: utils/server.c
cmd: utils/cmd.c
exit: utils/exit.c
signal: utils/signal.c

run.%: %
	./$*

run_all: $(UTILS) $(TESTS) $(RUN_TESTS)
	@echo "Tests ok"

%: %.o

clean:
	$(RM) *.o utils/*.o

distclean: clean
	$(RM) run_test $(UTILS) $(TESTS)

.PHONY: all clean distclean run_test
