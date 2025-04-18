cmake_minimum_required(VERSION 3.22.1)

project(logwrap)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


add_compile_options(-g -coverage -ftest-coverage)
add_link_options(-coverage)

add_executable(logwrap ../logwrap.c)


set(UTILS "utils/utils.c")

enable_testing()
include(CTest)

add_executable(server   utils/server.c)
add_executable(cmd      utils/cmd.c)
add_executable(exit     utils/exit.c)
add_executable(signal   utils/signal.c)
add_executable(append   utils/append.c)

add_executable(test_quit                ${UTILS} test_quit.c)
add_executable(test_simple              ${UTILS} test_simple.c)
add_executable(test_advanced            ${UTILS} test_advanced.c           utils/test_advanced.c)
add_executable(test_advanced_newline    ${UTILS} test_advanced_newline.c   utils/test_advanced.c)
add_executable(test_timed               ${UTILS} test_timed.c              utils/test_timed.c)
add_executable(test_timed_detach        ${UTILS} test_timed_detach.c       utils/test_timed.c)
add_executable(test_long                ${UTILS} test_long.c)
add_executable(test_lot_data            ${UTILS} test_lot_data.c)
add_executable(test_short_spam          ${UTILS} test_short_spam.c)
add_executable(test_usage               ${UTILS} test_usage.c)
add_executable(test_process_spam        ${UTILS} test_process_spam.c)
add_executable(test_end_write           ${UTILS} test_end_write.c)

add_test(NAME quit              COMMAND test_quit)
add_test(NAME simple            COMMAND test_simple)
add_test(NAME advanced          COMMAND test_advanced)
add_test(NAME advanced_newline  COMMAND test_advanced_newline)
add_test(NAME timed             COMMAND test_timed)
add_test(NAME timed_detach      COMMAND test_timed_detach)
add_test(NAME long              COMMAND test_long)
add_test(NAME lot_data          COMMAND test_lot_data)
add_test(NAME short_spam        COMMAND test_short_spam)
add_test(NAME usage             COMMAND test_usage)
add_test(NAME process_spam      COMMAND test_process_spam)
add_test(NAME end_write         COMMAND test_end_write)
