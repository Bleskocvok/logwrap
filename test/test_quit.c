#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

void test_case_exit()
{
    const char* argv[] = { PROG_PATH, "--", "false", "1", NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    int status;
    assert( waitpid( pid, &status, 0 ) > 0 );
    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 1 );
}

void test_case_exit_2()
{
    for ( int i = 0; i < 100; i++ )
    {
        char num_buf[ 128 ];
        snprintf( num_buf, sizeof num_buf, "%d", i );

        const char* argv[] = { PROG_PATH, "--", TEST_EXIT, num_buf, NULL };
        pid_t pid = fork_exec( argv[ 0 ], argv );

        int status;
        assert( waitpid( pid, &status, 0 ) > 0 );
        assert( WIFEXITED( status ) );
        assert( WEXITSTATUS( status ) == i );
    }
}

void test_case_signal()
{
    int sigs[] = { SIGKILL, SIGTERM, SIGINT, SIGPIPE };
    for ( int i = 0; i < 3; i++ )
    {
        int sig = sigs[ i ];

        char str[ 12 ] = { 0 };
        snprintf( str, sizeof str, "%d", sig );

        const char* argv[] = { PROG_PATH, "--", TEST_SIGNAL, str, NULL };
        pid_t pid = fork_exec( argv[ 0 ], argv );

        int status;
        assert( waitpid( pid, &status, 0 ) > 0 );
        assert( WIFEXITED( status ) );
        assert( WEXITSTATUS( status ) == 100 + sig );
    }
}

int main()
{
    test_case_exit();
    test_case_exit_2();
    test_case_signal();
}
