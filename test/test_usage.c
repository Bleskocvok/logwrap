#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

void test_param( const char* argv[], int exit_val )
{
    pid_t pid = fork_exec( argv[ 0 ], argv );

    int status;
    assert( waitpid( pid, &status, 0 ) > 0 );
    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == exit_val );
}

void test_case_params()
{
    const char* params[] = { "-h", "-j", "-d", "-p", "-n" };

    for ( unsigned i = 0; i < SIZE( params ); i++ )
    {
        const char* argv[] = { PROG_PATH, params[ i ], "--", TEST_EXIT, "0", NULL };
        test_param( argv, 0 );
    }
}

void test_case_bad_params()
{
    const char* params[] = { "-G", NULL };

    for ( unsigned i = 0; i < SIZE( params ); i++ )
    {
        const char* argv[] = { PROG_PATH, params[ i ], "--", TEST_EXIT, "0", NULL };
        test_param( argv, 1 );
    }
}

void test_case_bad_cmd()
{
    const char* argv[] = { PROG_PATH, "invalid cmd", NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    int status;
    assert( waitpid( pid, &status, 0 ) > 0 );
    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 200 );
}

int main()
{
    test_case_params();
    test_case_bad_params();
    test_case_bad_cmd();
}
