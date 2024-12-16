#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

int test_case_simple()
{
    int rv = 1;

    const char* CMD_OUTPUT_SOCKET = "./dump";
    const char* SERVER_INPUT_SOCKET = "./server_input";

    unlink( CMD_OUTPUT_SOCKET );
    unlink( SERVER_INPUT_SOCKET );

    link_t out;
    out.in = start_server( CMD_OUTPUT_SOCKET );
    if ( out.in == -1 )
        goto end;

    const char* argv[] = { PROG_PATH, TEST_SERVER, TEST_CMD, NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    out.out = start_connection( SERVER_INPUT_SOCKET );

    assert_put( out, "ahoj\n" );
    assert_get( out, "ahoj\n" );

    assert_put( out, "ooga booga\n");
    assert_get( out, "ooga booga\n" );

    assert_put( out, "aaa" );
    assert_put( out, "bbb" );
    assert_put( out, "ccc" );
    assert_put( out, "\n" );
    assert_get( out, "aaabbbccc\n" );

    // Closes the connection, so the ‹test_server› executable terminates
    // itself.
    close( out.out );
    out.out = -1;

    // We shouldn't get any extra output.
    assert_timeout_get( out, 1000 );

    int status;
    if ( waitpid( pid, &status, 0 ) == -1 )
        goto end;

    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 0 );

    rv = 0;
end:
    if ( out.in != -1 ) close( out.in );
    if ( out.out != -1 ) close( out.out );
    unlink( CMD_OUTPUT_SOCKET );
    return rv;
}

int main()
{
    return test_case_simple();
}
