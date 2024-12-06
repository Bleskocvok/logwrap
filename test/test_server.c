#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, bind, listen, accept
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write

#include <stdio.h>          // perror
#include <errno.h>          // errno

#define INPUT_SOCKET "./server_input"

int echo( int in_fd, int out_fd )
{
    char buf[ 256 ];
    int bytes;
    while ( ( bytes = read( in_fd, buf, sizeof buf ) ) > 0 )
    {
        int r = write( out_fd, buf, bytes );
        if ( r == -1 )
            return perror( "write" ), -1;

        if ( r != bytes )
            return fprintf( stderr, "written less\n" ), -1;
    }
    if ( bytes == -1 )
        return perror( "read" ), -1;

    return 0;
}

int main( void )
{
    int rv = 1;

    int client = -1;
    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return perror( "socket" ), 1;

    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", INPUT_SOCKET )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" );

    if ( unlink( addr.sun_path ) == -1 && errno != ENOENT )
    {
        perror( "unlink" );
        goto end;
    }

    if ( bind( sock_fd, ( struct sockaddr* )&addr, sizeof addr ) == -1 )
    {
        perror( "bind" );
        goto end;
    }

    if ( listen( sock_fd, 5 ) == -1 )
    {
        perror( "listen" );
        goto end;
    }

    // TODO: Only one accept is enough.
    // int client;
    // while ( ( client = accept( sock_fd, 0, 0 ) ) != -1 )
    // {
    //     int r = echo( client, 1 );
    //     close( client );
    //     if ( r == -1 )
    //         return close( sock_fd ), 1;
    // }
    client = accept( sock_fd, 0, 0 );
    if ( client == -1 )
    {
        perror( "accept" );
        goto end;
    }
    if ( echo( client, 1 ) == -1 )
        goto end;

    rv = 0;
end:
    if ( client != -1 ) close( client );
    if ( sock_fd != -1 ) close( sock_fd );
    unlink( INPUT_SOCKET );
    return rv;
}
