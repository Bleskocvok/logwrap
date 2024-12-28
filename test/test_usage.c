#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

void test_param( const char* param )
{
    const char* argv[] = { PROG_PATH, param, "--", TEST_EXIT, "0", NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    int status;
    assert( waitpid( pid, &status, 0 ) > 0 );
    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 0 );
}

void test_case_params()
{
    const char* params[] = { "-h", "-j", "-d", "-p", "-n" };

    for ( unsigned i = 0; i < SIZE( params ); i++ )
        test_param( params[ i ] );
}

int main()
{
    test_case_params();
}
