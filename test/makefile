CFLAGS = -std=c99 -Wall -Wextra

ifdef DEBUG
	CFLAGS += -g
endif

UTILS = utils/server utils/cmd utils/exit utils/signal utils/append
TESTS = test_quit \
		test_simple \
		test_advanced \
		test_advanced_newline \
		test_timed \
		test_timed_detach \
		test_long \
		test_lot_data \
		test_short_spam \
		test_usage \
		test_process_spam
RUN_TESTS = $(TESTS:%=run.%)

all: $(UTILS) $(TESTS)

run_test: utils/utils.o

test_quit:             utils/utils.c
test_simple:           utils/utils.c
test_advanced:         utils/utils.c utils/test_advanced.c
test_advanced_newline: utils/utils.c utils/test_advanced.c
test_timed:            utils/utils.c utils/test_timed.c
test_timed_detach:     utils/utils.c utils/test_timed.c
test_long:             utils/utils.c
test_lot_data:         utils/utils.c
test_short_spam:       utils/utils.c
test_usage:            utils/utils.c
test_process_spam:     utils/utils.c

utils/server: utils/server.c
utils/cmd: utils/cmd.c
utils/exit: utils/exit.c
utils/signal: utils/signal.c
utils/append: utils/append.c

tests: utils $(TESTS)

utils: $(UTILS)

run.%: %
	./$*

run_all: $(UTILS) $(TESTS) $(RUN_TESTS)
	@echo "Tests ok"

%: %.o

clean:
	$(RM) *.o utils/*.o

distclean: clean
	$(RM) run_test $(UTILS) $(TESTS)

.PHONY: all clean distclean run_test tests utils
