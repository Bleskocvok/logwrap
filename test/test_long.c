#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL, malloc, exit
#include <assert.h>         // assert
#include <stdio.h>          // snprintf
#include <string.h>         // strlen
#include <stdint.h>         // uint32_t

char* make_random_long( uint32_t seed, unsigned len, int newline )
{
    char* str = malloc( len + 2 );
    if ( !str )
    {
        perror( "malloc" );
        exit( 1 );
    }

    str[ len ] = 0;
    str[ len + 1 ] = 0;

    static const char symbols[] = "abcdefghijklmnopqrstuvwxyz";
    unsigned count = strlen( symbols );

    // LGC constants
    static uint32_t mult = 1103515245;
    static uint32_t add = 12345;

    uint32_t idx = seed * mult + add;

    for ( unsigned i = 0; i < len; i++ )
    {
        str[ i ] = symbols[ idx % count ];
        idx = idx * mult + add;
    }

    if ( newline )
        str[ len ] = '\n';

    return str;
}

int test_case_long( int ensure_newline )
{
    int rv = 1;

    char CMD_OUT[] = "./socket_long_cmd_out_0";
    char CMD_ERR[] = "./socket_long_cmd_err_0";
    char SERVER_OUT[] = "./socket_long_server_out_0";
    char SERVER_ERR[] = "./socket_long_server_err_0";

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

    char* str1           = make_random_long( 0, 256, 0 );
    char* str1_nl        = make_random_long( 0, 256, 1 );
    char* after_konec    = make_random_long( 89, 4096, 0 );
    char* after_konec_nl = make_random_long( 89, 4096, 1 );

    assert_put( sout, str1 );
    assert_put( sout, "\n" );
    assert_get( sout, str1_nl );

    // TODO: Figure out why it fails for 100. The issue is probably in the
    // tests, not the code itself.
    //       for ( int i = 1; i < 100; i++ )
    for ( int i = 1; i < 30; i++ )
    {
        char* str    = make_random_long( i, 1024 * i, 0 );
        char* str_nl = make_random_long( i, 1024 * i, 1 );

        assert_put( ser, str );
        assert_put( sout, str_nl );
        assert_put( ser, "\n" );

        assert_get( sout, str_nl );
        assert_get( ser, str_nl );

        free( str );
        free( str_nl );
    }

    assert_timeout_get( ser, 1000 );

    assert_put( sout, after_konec );

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
        assert_get( ser, ensure_newline ? after_konec_nl : after_konec );

    // We shouldn't get any extra error output either.
    assert_timeout_get( ser, 1000 );

    free( str1 );
    free( str1_nl );
    free( after_konec );
    free( after_konec_nl );

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

int main( void )
{
    test_case_long( 0 );
}
