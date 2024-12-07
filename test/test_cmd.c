#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, connect
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write

#include <stdio.h>          // perror, snprintf

#define OUTPUT_SOCKET "./dump"

int main( int argc, char** argv )
{
    const char* filename = argc >= 2 ? argv[ 1 ] : OUTPUT_SOCKET;

    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", filename )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" );

    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return perror( "socket" ), 1;

    if ( connect( sock_fd, ( struct sockaddr* )&addr, sizeof addr ) == -1 )
        // TODO: Doesn't really matter, but perhaps be cleaner about closing
        // things, or use ‹err› or similar.
        return perror( "connect" ), 1;

    char buf[ 256 ];
    int bytes;

    // Echo input from stdin to the socket.
    while ( ( bytes = read( 0, buf, sizeof buf ) ) > 0 )
    {
        int r = write( sock_fd, buf, bytes );
        if ( r == -1 )
            return perror( "write" ), 1;

        if ( r != bytes )
            return fprintf( stderr, "written less\n" ), 1;
    }

    close( sock_fd );

    return 0;
}
