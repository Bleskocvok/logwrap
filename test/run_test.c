#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, bind, listen, accept
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write
#include <unistd.h>         // fork
#include <sys/wait.h>       // waitpid
#include <time.h>           // nanosleep
#include <poll.h>           // poll

#include <stdio.h>          // perror
#include <errno.h>          // errno
#include <stdlib.h>         // exit, NULL
#include <assert.h>         // assert
#include <string.h>         // memcmp, strerror, strlen

#define PROG_PATH "../logwrap"

typedef struct
{
    int in;
    int out;

} link_t;

int start_server( const char* cmd_output_socket );
int start_connection( const char* server_input );
void error( const char* str );
void assert_put( link_t lnk, const char* str );
void assert_get( link_t lnk, const char* expected );
void assert_timeout_get( link_t lnk, int timeout_ms );
pid_t fork_exec( const char* cmd, char* const* argv );

void test_case_exit()
{
    char* const argv[] = { PROG_PATH, "--", "false", "1", NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    int status;
    assert( waitpid( pid, &status, 0 ) > 0 );
    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 1 );
}

void test_case_exit_2()
{
    for ( int i = 0; i < 100; i++ )
    {
        char num_buf[ 128 ];
        snprintf( num_buf, sizeof num_buf, "%d", i );

        char* const argv[] = { PROG_PATH, "--", "./test_exit", num_buf, NULL };
        pid_t pid = fork_exec( argv[ 0 ], argv );

        int status;
        assert( waitpid( pid, &status, 0 ) > 0 );
        assert( WIFEXITED( status ) );
        assert( WEXITSTATUS( status ) == i );
    }
}

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

    char* const argv[] = { PROG_PATH, "./test_server", "./test_cmd", NULL };
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

int test_case_advanced()
{
    int rv = 1;

    char CMD_OUT[] = "./cmd_out";
    char CMD_ERR[] = "./cmd_err";
    char SERVER_OUT[] = "./server_out";
    char SERVER_ERR[] = "./server_err";

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

    char* const argv[] = { PROG_PATH, "--", "./test_server", SERVER_OUT, SERVER_ERR,
                           "--", "./test_cmd", CMD_OUT,
                           "--", "./test_cmd", CMD_ERR,
                           NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    sout.out = start_connection( SERVER_OUT );
    ser.out = start_connection( SERVER_ERR );

    assert_put( sout, "ahoj\n" );
    assert_get( sout, "ahoj\n" );

    assert_put( ser, "ooga booga\n");
    assert_get( ser, "ooga booga\n" );

    assert_put( sout, "aaa" );
    assert_put( ser,  "ááá" );
    assert_put( sout, "bbb" );
    assert_put( ser,  "ččč" );
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

    assert_get( ser, "after konec no newline\n" );
    // assert_get( ser, "after konec no newline" );

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

void ms_sleep( int ms )
{
    int sec = ms / 1000;
    int ms_ = ms % 1000;
    struct timespec ts = { .tv_sec = sec, .tv_nsec = 1000 * 1000 * ms_ };
    if ( nanosleep( &ts, NULL ) == -1 )
    {
        perror( "nanosleep" );
        exit( 1 );
    }
}

int test_case_timed( int detach )
{
    int rv = 1;

    char CMD_OUT[] = "./cmd_out";
    char CMD_ERR[] = "./cmd_err";
    char SERVER_OUT[] = "./server_out";
    char SERVER_ERR[] = "./server_err";

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

    char* argv[ 16 ] = { 0 };
    if ( detach )
    {
        char* const args[] = { PROG_PATH, "-s", "1",
                                          "--", "./test_server", SERVER_OUT, SERVER_ERR,
                                          "--", "./test_cmd", CMD_OUT,
                                          "--", "./test_cmd", CMD_ERR,
                                          NULL, NULL };
        memcpy( argv, args, sizeof args );
    }
    else
    {
        char* const args[] = { PROG_PATH, "-s", "1", "-d",
                                          "--", "./test_server", SERVER_OUT, SERVER_ERR,
                                          "--", "./test_cmd", CMD_OUT,
                                          "--", "./test_cmd", CMD_ERR,
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

int main( void )
{
    test_case_exit();
    test_case_exit_2();

    test_case_simple();
    test_case_advanced();
    test_case_timed( 0 );
    test_case_timed( 1 );
}

void error( const char* str )
{
    fprintf( stderr, "%s: %s\n", str, strerror( errno ) );
    exit( 1 );
}

int poll_read_fd( int fd, int timeout_ms )
{
    struct pollfd pfd = { .fd = fd, .events = POLLIN, };
    return poll( &pfd, 1, timeout_ms );
}

int get( int sock_fd, char* buf, int size, int timeout_ms )
{
    if ( poll_read_fd( sock_fd, timeout_ms) != 1 )
        return -2;

    int rv = -1;
    int client = accept( sock_fd, 0, 0 );
    if ( client == -1 )
        return perror("accept"), -1;

    if ( poll_read_fd( client, timeout_ms) != 1 )
    {
        rv = -2;
        goto end;
    }

    char discard[ 256 ];
    int total = 0;
    int bytes;
    while ( ( bytes = read( client,
                            total >= size ? discard : buf + total,
                            total >= size ? ( int )sizeof discard : size - total ) ) > 0 )
    {
        total += bytes;
    }
    if ( bytes == -1 )
        goto end;

    rv = total;
end:
    close( client );
    return rv;
}

int put( int sock_fd, const char* buf, int size )
{
    int written = write( sock_fd, buf, size );
    if ( written == -1 )
        return perror( "write" ), -1;
    if ( written != size )
        return fprintf( stderr, "written less bytes\n" );
    return 0;
}

int start_server( const char* cmd_output_socket )
{

    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", cmd_output_socket )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" );

    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return perror( "socket" ), -1;

    if ( unlink( addr.sun_path ) == -1 && errno != ENOENT )
    {
        perror( "unlink" );
        goto bad;
    }

    if ( bind( sock_fd, ( struct sockaddr* )&addr, sizeof addr ) == -1 )
    {
        perror( "bind" );
        goto bad;
    }

    if ( listen( sock_fd, 5 ) == -1 )
    {
        perror( "listen" );
        goto bad;
    }

    return sock_fd;

bad:
    if ( sock_fd != -1 ) close( sock_fd );
    unlink( addr.sun_path );
    return -1;
}

int start_connection( const char* server_input )
{
    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return perror( "socket" ), 1;

    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", server_input )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" );

    while ( 1 )
    {
        int c = connect( sock_fd, ( struct sockaddr* )&addr, sizeof addr );
        if ( c == -1 && errno != ECONNREFUSED && errno != ENOENT )
            return perror( "run: connect" ), 1;
        if ( c == 0 )
            break;

        /* Wait 100ms. */
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
        if ( nanosleep( &ts, NULL ) == -1 )
            perror( "nanosleep" );
    }
    return sock_fd;
}

pid_t fork_exec( const char* cmd, char* const* argv )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        execv( cmd, argv );
        perror( "execv" );
        exit( 200 );
    }

    return pid;
}

void print_escaped( const char* buf, int size )
{
    for ( int i = 0; i < size; i++ )
    {
        if ( buf[ i ] == '\n' )
            fprintf( stderr, "%s", "\\n" );
        else
            fputc( buf[ i ], stderr );
    }
}

void assert_put( link_t lnk, const char* str )
{
    assert( put( lnk.out, str, strlen( str ) ) == 0 );
}

void assert_timeout_get( link_t lnk, int timeout_ms )
{
    char buf[ 4096 ];
    int got = get( lnk.in, buf, sizeof buf, timeout_ms );
    assert( got == -2 );
}

void assert_get( link_t lnk, const char* expected )
{
    int size = strlen( expected );

    char buf[ 4096 ];
    int got = get( lnk.in, buf, sizeof buf, 5000 );
    if ( got < 0 )
    {
        fprintf( stderr, "%s while waiting for: \"", got == -2 ? "timeout" : "error" );
        print_escaped( expected, size);
        fprintf( stderr, "\"\n" );
        exit( 1 );
    }

    if ( got > ( int )sizeof buf
            || got != size
            || memcmp( buf, expected, size ) != 0 )
    {
        if ( got > ( int )sizeof buf )
            got = sizeof buf;

        fprintf( stderr, "got:      \"" ); print_escaped( buf, got );       fprintf( stderr, "\"\n" );
        fprintf( stderr, "expected: \"" ); print_escaped( expected, size ); fprintf( stderr, "\"\n" );
        exit( 1 );
    }
}

