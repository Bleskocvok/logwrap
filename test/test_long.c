#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL, malloc, exit
#include <assert.h>         // assert
#include <stdint.h>         // uint32_t

void test_case_long( int ensure_newline )
{
    config_t c;

    if ( ensure_newline )
        config_init( &c, "./socket_long_cmd_out_0", "./socket_long_cmd_err_0",
                         "./socket_long_server_out_0", "./socket_long_server_err_0" );
    else
        config_init( &c, "./socket_long_cmd_out_1", "./socket_long_cmd_err_1",
                         "./socket_long_server_out_1", "./socket_long_server_err_1" );

    c.args = new_args();
    args_push( &c.args, PROG_PATH );
    if ( ensure_newline )
        args_push( &c.args, "-n" );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_SERVER );
    args_push( &c.args, c.SERVER_OUT );
    args_push( &c.args, c.SERVER_ERR );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_CMD );
    args_push( &c.args, c.CMD_OUT );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_CMD );
    args_push( &c.args, c.CMD_ERR );

    begin( &c );

    link_t sout = c.sout;
    link_t ser = c.ser;

    char* str1           = make_random_long( 0, 256, 0 );
    char* str1_nl        = make_random_long( 0, 256, 1 );
    char* after_konec    = make_random_long( 89, 4096, 0 );
    char* after_konec_nl = make_random_long( 89, 4096, 1 );

    assert_put( sout, str1 );
    assert_put( sout, "\n" );
    assert_get( sout, str1_nl );

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

    assert_put( ser, after_konec );

    // Closes one connection.
    close( sout.out );
    sout.out = -1;

    // We shouldn't get any extra output.
    assert_timeout_get( sout, 1000 );

    // Closes the other connection, so the ‹test_server› executable terminates
    // itself.
    close( ser.out );
    ser.out = -1;

    assert_get( ser, ensure_newline ? after_konec_nl : after_konec );

    // We shouldn't get any extra error output either.
    assert_timeout_get( ser, 1000 );

    free( str1 );
    free( str1_nl );
    free( after_konec );
    free( after_konec_nl );

    end( &c );
}

int main( void )
{
    test_case_long( 0 );
    test_case_long( 1 );
}
