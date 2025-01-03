#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <assert.h>         // assert
#include <stdint.h>         // uint32_t
#include <string.h>         // memset

void test_case_end_write()
{
    config_t c;

    config_init( &c, "./socket_end_write_cmd_out_0", "./socket_end_write_cmd_err_0",
                     "./socket_end_write_server_out_0", "./socket_end_write_server_err_0" );

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

    assert_put( ser,  "some unfinished line to stderr" );
    assert_put( sout, "some unfinished line to stdout" );

    assert_timeout_get( ser, 1000 );
    assert_timeout_get( sout, 10 );

    // Close both connections, so the ‹test_server› executable terminates
    // itself.
    close( sout.out );
    close( ser.out );
    sout.out = -1;
    ser.out = -1;

    assert_get( ser,  "some unfinished line to stderr" );
    assert_get( sout, "some unfinished line to stdout" );

    // We shouldn't get any extra output.
    assert_timeout_get( sout, 1000 );
    assert_timeout_get( ser, 10 );

    end( &c );
}

int main( void )
{
    test_case_end_write();
}
