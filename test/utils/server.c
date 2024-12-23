#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, bind, listen, accept
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write
#include <poll.h>           // poll

// debug
#include <sys/stat.h>       // open
#include <fcntl.h>          // open
#include <unistd.h>         // dup2
#include <stdio.h>          // dprintf
#include <stdlib.h>         // getenv

#include <stdio.h>          // perror
#include <errno.h>          // errno

#define INPUT_SOCKET "./socket_server_input"

void init_debug( const char* filename )
{
    if ( getenv( "LOGWRAP_DEBUG" ) == NULL )
        return;
    dup2( 1, 3 );
    int f = open( filename, O_WRONLY | O_TRUNC | O_CREAT, 0666 );
    dup2( f, 3 );
    close( f );
}

int echo( int in_fd, int out_fd )
{
    char buf[ 256 ];
    int bytes;
    if ( ( bytes = read( in_fd, buf, sizeof buf ) ) > 0 )
    {
        dprintf( 3, "server: read(%d -> %d) \"%.*s\"\n", in_fd, out_fd, bytes, buf );
        int r = write( out_fd, buf, bytes );
        if ( r == -1 )
            return perror( "write" ), -1;

        if ( r != bytes )
            return fprintf( stderr, "written less\n" ), -1;
    }
    if ( bytes == -1 )
        return perror( "read" ), -1;

    return bytes;
}

int start_server( const char* filename )
{
    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", filename )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" ), -1;

    if ( unlink( addr.sun_path ) == -1 && errno != ENOENT )
    {
        perror( "unlink" );
        return -1;
    }

    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return perror( "socket" ), -1;

    if ( bind( sock_fd, ( struct sockaddr* )&addr, sizeof addr ) == -1 )
    {
        perror( "bind" );
        close( sock_fd );
        return -1;
    }

    if ( listen( sock_fd, 5 ) == -1 )
    {
        perror( "listen" );
        close( sock_fd );
        return -1;
    }

    return sock_fd;
}

int main( int argc, char** argv )
{
    init_debug( "debug_server" );

    const char* filenames[] = { INPUT_SOCKET, NULL };
    for ( int i = 0; i < 2; i++ )
        if ( argc >= i + 2 )
            filenames[ i ] = argv[ i + 1 ];

    int rv = 1;

    int clients[ 2 ] = { -1, -1 };
    int sock_fds[ 2 ] = { -1, -1 };
    int used = 1 + ( argc >= 3 );

    for ( int i = 0; i < used; i++ )
    {
        sock_fds[ i ] = start_server( filenames[ i ] );
        if ( sock_fds[ i ] == -1 )
            goto end;
    }

    for ( int i = 0; i < used; i++ )
    {
        clients[ i ] = accept( sock_fds[ i ], 0, 0 );
        if ( clients[ i ] == -1 )
        {
            perror( "accept" );
            goto end;
        }
    }

    struct pollfd fds[ 2 ] = { { .fd = clients[ 0 ], .events = POLLIN, },
                               { .fd = clients[ 1 ], .events = POLLIN, }, };
    while ( 1 )
    {
        if ( poll( fds, used, -1 ) == -1 )
        {
            perror( "poll" );
            goto end;
        }

        for ( int i = 0; i < used; i++ )
        {
            if ( fds[ i ].fd == -1 )
                continue;

            if ( fds[ i ].revents & ( POLLIN | POLLHUP ) )
            {
                int wr = echo( fds[ i ].fd, i + 1 );
                if ( wr == -1 ) goto end;
                if ( wr == 0 )
                {
                    close( fds[ i ].fd );
                    fds[ i ].fd = -1;
                }
            }
        }

        int is_open = 0;
        for ( int i = 0; i < used; i++ )
            if ( fds[ i ].fd != -1 )
                is_open = 1;
        if ( !is_open )
            break;
    }

    rv = 0;
end:
    for ( int i = 0; i < used; i++ )
    {
        if ( sock_fds[ i ] != -1 ) close( sock_fds[ i ] );
        if ( clients[ i ] != -1 ) close( clients[ i ] );
        unlink( filenames[ i ] );
    }
    return rv;
}
