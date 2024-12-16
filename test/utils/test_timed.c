#define _POSIX_C_SOURCE 200809L

#include "utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

int test_case_timed( int detach )
{
    int rv = 1;

    char CMD_OUT[] = "./timed_cmd_out_0";
    char CMD_ERR[] = "./timed_cmd_err_0";
    char SERVER_OUT[] = "./timed_server_out_0";
    char SERVER_ERR[] = "./timed_server_err_0";

    if ( detach )
    {
        CMD_OUT[ sizeof CMD_OUT - 2 ] = '1';
        CMD_ERR[ sizeof CMD_ERR - 2 ] = '1';
        SERVER_OUT[ sizeof SERVER_OUT - 2 ] = '1';
        SERVER_ERR[ sizeof SERVER_ERR - 2 ] = '1';
    }

    unlink( CMD_OUT );
    unlink( CMD_ERR );
    unlink( SERVER_OUT );
    unlink( SERVER_ERR );

    link_t sout = { .in = -1, .out = -1, };
    link_t ser = { .in = -1, .out = -1, };;

    sout.in = start_server( CMD_OUT );
    if ( sout.in == -1 ) goto end;

    ser.in = start_server( CMD_ERR );
    if ( ser.in == -1 ) goto end;

    const char* argv[ 16 ] = { 0 };
    if ( detach )
    {
        const char* args[] = { PROG_PATH, "-s", "1",
                                          "--", TEST_SERVER, SERVER_OUT, SERVER_ERR,
                                          "--", TEST_CMD, CMD_OUT,
                                          "--", TEST_CMD, CMD_ERR,
                                          NULL, NULL };
        memcpy( argv, args, sizeof args );
    }
    else
    {
        const char* args[] = { PROG_PATH, "-s", "1", "-d",
                                          "--", TEST_SERVER, SERVER_OUT, SERVER_ERR,
                                          "--", TEST_CMD, CMD_OUT,
                                          "--", TEST_CMD, CMD_ERR,
                                          detach ? "-d" : NULL,
                                          NULL };
        memcpy( argv, args, sizeof args );
    }
    pid_t pid = fork_exec( argv[ 0 ], argv );

    sout.out = start_connection( SERVER_OUT );
    ser.out = start_connection( SERVER_ERR );

    assert_put( sout, "ahoj\n" );
    ms_sleep( 1000 );
    assert_get( sout, "ahoj\n" );

    assert_put( sout, "line1\n" );
    assert_put( sout, "line2\n" );
    assert_put( sout, "line3\n" );
    assert_timeout_get( sout, 100 );

    assert_get( sout, "line1\nline2\nline3\n" );
    assert_timeout_get( sout, 1000 );

    assert_put( sout, "line1\n" );
    assert_put( sout, "line2\n" );
    assert_put( sout, "line3\n" );
    assert_put( ser, "EPIPE\n" );
    assert_put( ser, "EAGAIN" );
    assert_put( ser, " ECONNREFUSED\n" );
    assert_put( ser, "ENOENT\n" );
    assert_timeout_get( sout, 100 );
    assert_timeout_get( ser, 100 );

    assert_get( sout, "line1\nline2\nline3\n" );
    assert_get( ser, "EPIPE\nEAGAIN ECONNREFUSED\nENOENT\n" );
    assert_timeout_get( sout, 2000 );
    assert_timeout_get( ser, 2000 );

    // Closes one connection.
    close( sout.out );
    sout.out = -1;

    // We shouldn't get any extra output.
    assert_timeout_get( sout, 1000 );

    // Closes the other connection, so the ‹test_server› executable terminates
    // itself.
    close( ser.out );
    ser.out = -1;

    // We shouldn't get any extra error output either.
    assert_timeout_get( ser, 1000 );

    int status;
    if ( waitpid( pid, &status, 0 ) == -1 )
        goto end;

    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 0 );

    rv = 0;
end:
    if ( sout.in != -1 ) close( sout.in );
    if ( sout.out != -1 ) close( sout.out );
    if ( ser.in != -1 ) close( ser.in );
    if ( ser.out != -1 ) close( ser.out );
    unlink( CMD_OUT );
    unlink( CMD_ERR );
    unlink( SERVER_OUT );
    unlink( SERVER_ERR );
    return rv;
}


