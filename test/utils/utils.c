#define _POSIX_C_SOURCE 200809L

#include "utils.h"

#include <sys/socket.h>     // sockaddr_un, socket, bind, listen, accept
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write
#include <unistd.h>         // fork
#include <sys/wait.h>       // waitpid
#include <time.h>           // nanosleep
#include <poll.h>           // poll

#include <stdio.h>          // perror
#include <errno.h>          // errno
#include <stdlib.h>         // exit, NULL, malloc
#include <assert.h>         // assert
#include <string.h>         // memcmp, strerror, strlen, memset
#include <stdio.h>          // snprintf
#include <string.h>         // strlen

args_t new_args()
{
    args_t args;
    memset( args.data, 0, sizeof args.data );
    args.size = 0;
    return args;
}

void args_push( args_t* args, const char* arg )
{
    if ( args->size + 1 >= ( int )( sizeof args->data / sizeof *args->data ) )
        assert( 0 );
    args->data[ args->size++ ] = arg;
}

void args_append( args_t* args, const char* elems[], int size )
{
    for ( int i = 0; i < size; i++ )
        args_push( args, elems[ i ] );
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
    int p;
    if ( ( p = poll_read_fd( sock_fd, timeout_ms) ) != 1 )
    {
        if ( p < 0 )
            return p;
        else
            return -2;
    }

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

pid_t fork_exec( const char* cmd, const char* argv[] )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        execv( cmd, ( char* const* )argv );
        perror( "execv" );
        exit( 200 );
    }

    if ( pid == -1 )
        perror( "utils: fork" ), exit( 1 );

    return pid;
}

void print_escaped( const char* buf, int size )
{
    for ( int i = 0; i < size; i++ )
    {
        if ( size > 80 && ( i >= 40 && i < size - 40 ) )
        {
            if ( i == 50 )
                fprintf( stderr, "\" ... \"" );
            continue;
        }

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

    int buf_size = size + 100;
    char* buf = malloc( buf_size );
    if ( !buf )
        perror( "malloc" ), exit( 1 );

    int got = get( lnk.in, buf, buf_size, 5000 );
    if ( got < 0 )
    {
        fprintf( stderr, "%s while waiting for (%d): \"", got == -2 ? "timeout" : "error", size );
        print_escaped( expected, size);
        fprintf( stderr, "\"\n" );
        exit( 1 );
    }

    if ( got > ( int )size
            || got != size
            || memcmp( buf, expected, size ) != 0 )
    {
        fprintf( stderr, "got (%d):      \"", got ); print_escaped( buf, got );       fprintf( stderr, "\"\n" );
        fprintf( stderr, "expected (%d): \"", size ); print_escaped( expected, size ); fprintf( stderr, "\"\n" );
        exit( 1 );
    }

    free( buf );
}

void config_init( config_t* c, const char* co, const char* ce,
                               const char* so, const char* se )
{
    int size = sizeof c->CMD_OUT;
    snprintf( c->CMD_OUT, size, "%s", co );
    snprintf( c->CMD_ERR, size, "%s", ce );
    snprintf( c->SERVER_OUT, size, "%s", so );
    snprintf( c->SERVER_ERR, size, "%s", se );
}

void begin( config_t* c )
{
    unlink( c->CMD_OUT );
    unlink( c->CMD_ERR );
    unlink( c->SERVER_OUT );
    unlink( c->SERVER_ERR );

    c->sout.in = start_server( c->CMD_OUT );
    if ( c->sout.in == -1 ) goto end;

    c->ser.in = start_server( c->CMD_ERR );
    if ( c->ser.in == -1 ) goto end;

    c->pid = fork_exec( c->args.data[ 0 ], c->args.data );

    c->sout.out = start_connection( c->SERVER_OUT );
    c->ser.out = start_connection( c->SERVER_ERR );
    return;

end:
    fprintf( stderr, "begin: error\n" );
    exit( 1 );
}

void end( config_t* c )
{
    unlink( c->CMD_OUT );
    unlink( c->CMD_ERR );
    unlink( c->SERVER_OUT );
    unlink( c->SERVER_ERR );

    int status;
    if ( waitpid( c->pid, &status, 0 ) == -1 )
        goto end;

    assert( WIFEXITED( status ) );
    assert( WEXITSTATUS( status ) == 0 );

end:
    if ( c->sout.in != -1 ) close( c->sout.in );
    if ( c->sout.out != -1 ) close( c->sout.out );
    if ( c->ser.in != -1 ) close( c->ser.in );
    if ( c->ser.out != -1 ) close( c->ser.out );
    unlink( c->CMD_OUT );
    unlink( c->CMD_ERR );
    unlink( c->SERVER_OUT );
    unlink( c->SERVER_ERR );
}

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
