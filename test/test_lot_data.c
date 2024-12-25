#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <stdlib.h>         // NULL, malloc, exit
#include <assert.h>         // assert
#include <stdint.h>         // uint32_t
#include <string.h>         // memset
#include <stdio.h>          // printf

void test_case_lot_data()
{
    config_t c;

    config_init( &c, "./socket_lot_data_cmd_out_0", "./socket_lot_data_cmd_err_0",
                     "./socket_lot_data_server_out_0", "./socket_lot_data_server_err_0" );

    c.args = new_args();
    args_push( &c.args, PROG_PATH );
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

    for ( int i = 0; i < 3000; i++ )
    {
        char* str_nl = make_random_long( i, 1024 + i % 1024, 1 );

        assert_put( ser, str_nl );
        assert_put( sout, str_nl );

        assert_get( ser, str_nl );
        assert_get( sout, str_nl );

        free( str_nl );
    }

    assert_timeout_get( ser, 1000 );

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

    end( &c );
}

void test_case_short_spam()
{
    config_t c;

    config_init( &c, "./socket_short_spam_cmd_out_0", "./socket_short_spam_cmd_err_0",
                     "./socket_short_spam_server_out_0", "./socket_short_spam_server_err_0" );

    c.args = new_args();
    args_push( &c.args, PROG_PATH );
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

    for ( int i = 1; i < 20; i++ )
    {
        int newlines = i;
        char str[ newlines + 1 ];
        memset( str, '\n', newlines );
        str[ newlines ] = 0;

// printf( "newlines=%d\n", newlines );

        assert_put( ser, str );
        assert_put( sout, str );
        for ( int j = 0; j < newlines; j++ )
        {
// fprintf(stderr, "j=%d    ", j);
// fprintf(stderr, "\tget e %d: ", j); fflush(stderr);
            assert_get( ser, "\n" );
// fprintf(stderr, "ok");
// fprintf(stderr, "    get o %d: ", j); fflush(stderr);
            assert_get( sout, "\n" );
// fprintf(stderr, "ok\n");
        }
    }

    assert_timeout_get( ser, 1000 );

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

    end( &c );
}

int main( void )
{
    test_case_lot_data();
    test_case_short_spam();
}
