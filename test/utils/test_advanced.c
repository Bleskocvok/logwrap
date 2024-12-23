#define _POSIX_C_SOURCE 200809L

#include "utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // memcpy

int test_case_advanced( int ensure_newline )
{
    int rv = 1;

    char CMD_OUT[] = "./socket_advanced_cmd_out_0";
    char CMD_ERR[] = "./socket_advanced_cmd_err_0";
    char SERVER_OUT[] = "./socket_advanced_server_out_0";
    char SERVER_ERR[] = "./socket_advanced_server_err_0";

    if ( ensure_newline )
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

    // char* const argv[] = { PROG_PATH, "--", "./test_server", SERVER_OUT, SERVER_ERR,
    //                        "--", "./test_cmd", CMD_OUT,
    //                        "--", "./test_cmd", CMD_ERR,
    //                        NULL };
    args_t args = new_args();
    args_push( &args, PROG_PATH );
    if ( ensure_newline )
        args_push( &args, "-n" );
    args_push( &args, "--" );
    args_push( &args, TEST_SERVER );
    args_push( &args, SERVER_OUT );
    args_push( &args, SERVER_ERR );
    args_push( &args, "--" );
    args_push( &args, TEST_CMD );
    args_push( &args, CMD_OUT );
    args_push( &args, "--" );
    args_push( &args, TEST_CMD );
    args_push( &args, CMD_ERR );

    pid_t pid = fork_exec( args.data[ 0 ], args.data );

    sout.out = start_connection( SERVER_OUT );
    ser.out = start_connection( SERVER_ERR );

    assert_put( sout, "ahoj\n" );
    assert_get( sout, "ahoj\n" );

    assert_put( ser, "ooga booga\n");

    assert_put( sout, "aaa" );
    assert_put( ser,  "ááá" );
    assert_put( sout, "bbb" );
    assert_put( ser,  "ččč" );

    assert_get( ser, "ooga booga\n" );

    assert_put( sout, "ccc" );
    assert_put( ser,  "ďďď" );
    assert_put( sout, "\n" );
    assert_get( sout, "aaabbbccc\n" );

    assert_timeout_get( ser, 1000 );
    assert_put( ser, "\n" );
    assert_get( ser, "áááčččďďď\n" );

    assert_put( ser, "after konec no newline" );

    // Closes one connection.
    close( sout.out );
    sout.out = -1;

    // We shouldn't get any extra output.
    assert_timeout_get( sout, 1000 );

    // Closes the other connection, so the ‹test_server› executable terminates
    // itself.
    close( ser.out );
    ser.out = -1;

    if ( ensure_newline )
        assert_get( ser, "after konec no newline\n" );
    else
        assert_get( ser, "after konec no newline" );

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
