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

#define CMD_OUTPUT_SOCKET   "./dump"
#define SERVER_INPUT_SOCKET "./server_input"

int start_server( const char* cmd_output_socket );
int start_connection( const char* server_input );
void error( const char* str );
void assert_put( int fd, const char* str );
void assert_get( int fd, const char* expected );
pid_t fork_exec( const char* cmd, char* const* argv );

int main( void )
{
    int rv = 1;

    int out_fd = -1;
    int in_fd = start_server( CMD_OUTPUT_SOCKET );
    if ( in_fd == -1 )
        goto end;

    char* const argv[] = { "../logwrap", "./test_server", "./test_cmd", NULL };
    pid_t pid = fork_exec( argv[ 0 ], argv );

    out_fd = start_connection( SERVER_INPUT_SOCKET );

    assert_put( out_fd, "ahoj\n" );
    assert_get( in_fd, "ahoj\n" );

    assert_put( out_fd, "ooga booga\n");
    assert_get( in_fd, "ooga booga\n" );

    // Closes the connection, so the ‹test_server› executable terminates
    // itself.
    close( out_fd );
    out_fd = -1;

    int status;
    if ( waitpid( pid, &status, 0 ) == -1 )
        goto end;

    rv = 0;
end:
    if ( in_fd != -1 ) close( in_fd );
    if ( out_fd != -1 ) close( out_fd );
    unlink( CMD_OUTPUT_SOCKET );
    return rv;
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
        return fprintf( stderr, "timeout\n" ), -1;

    int rv = -1;
    int client = accept( sock_fd, 0, 0 );
    if ( client == -1 )
        return perror("accept"), -1;

    if ( poll_read_fd( client, timeout_ms) != 1 )
    {
        fprintf( stderr, "timeout\n" );
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

void assert_put( int fd, const char* str )
{
    assert( put( fd, str, strlen( str ) ) == 0 );
}

void assert_get( int fd, const char* expected )
{
    int size = strlen( expected );

    char buf[ 4096 ];
    int got = get( fd, buf, sizeof buf, 50000 );
    if ( got == -1 )
        exit( 1 );

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

