#define _POSIX_C_SOURCE 200809L

#include "utils/utils.h"

#include <unistd.h>         // close
#include <sys/wait.h>       // waitpid

#include <assert.h>         // assert
#include <stdint.h>         // uint32_t
#include <string.h>         // memset

#include <sys/stat.h>       // open
#include <fcntl.h>          // open

int count_newlines( const char* file )
{
    int fd = open( file, O_RDONLY );
    if ( fd == -1 )
        return -1;

    int count = 0;

    char buf[ 128 ];
    int bytes;

    while ( ( bytes = read( fd, buf, sizeof buf ) ) > 0 )
    {
        for ( int i = 0; i < bytes; i++ )
            if ( buf[ i ] != '\n' )
            {
                count = -1;
                goto end;
            }

        count += bytes;
    }
end:
    close( fd );
    return count;
}

void test_case_process_spam()
{
    config_t c;

    config_init( &c, "./socket_process_spam_cmd_out_0", "./socket_process_spam_cmd_err_0",
                     "./socket_process_spam_server_out_0", "./socket_process_spam_server_err_0" );

    const char* out_file = "./file_process_spam_out";
    const char* err_file = "./file_process_spam_err";
    close( open( out_file, O_CREAT | O_TRUNC, 0666 ) );
    close( open( err_file, O_CREAT | O_TRUNC, 0666 ) );

    c.args = new_args();
    args_push( &c.args, PROG_PATH );
    args_push( &c.args, "-d" );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_SERVER );
    args_push( &c.args, c.SERVER_OUT );
    args_push( &c.args, c.SERVER_ERR );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_APPEND );
    args_push( &c.args, out_file );
    args_push( &c.args, "--" );
    args_push( &c.args, TEST_APPEND );
    args_push( &c.args, err_file );

    begin( &c );

    link_t sout = c.sout;
    link_t ser = c.ser;

    int newlines = 2000;

    char str[ newlines + 1 ];
    memset( str, '\n', newlines );
    str[ newlines ] = 0;

    assert_put( ser, str );
    assert_put( sout, str );

    // Closes one connection.
    close( sout.out );
    sout.out = -1;

    // Closes the other connection, so the ‹test_server› executable terminates
    // itself.
    close( ser.out );
    ser.out = -1;

    end( &c );

    int out_nls = count_newlines( out_file );
    int err_nls = count_newlines( err_file );

    assert( out_nls == newlines );
    assert( err_nls == newlines );

    unlink( err_file );
    unlink( out_file );
}

int main( void )
{
    test_case_process_spam();
}
