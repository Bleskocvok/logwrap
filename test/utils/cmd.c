#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, connect
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write

#include <stdio.h>          // perror, snprintf

// debug
#include <sys/stat.h>       // open
#include <fcntl.h>          // open
#include <unistd.h>         // dup2
#include <stdio.h>          // dprintf
#include <stdlib.h>         // getenv, mkstemp
#include <errno.h>          // errno
#include <string.h>         // strerror

#define OUTPUT_SOCKET "./socket_dump"

void init_debug( char filename[] )
{
    if ( getenv( "LOGWRAP_DEBUG" ) == NULL )
        return;
    dup2( 1, 3 );
    int f = mkstemp( filename );
    dup2( f, 3 );
    close( f );
}

void cerror( const char* str )
{
    int e = errno;
    fprintf( stderr, "%s (%d): %s\n", str, e, strerror( e ) );
    fflush( stderr );
    dprintf( 3, "cmd: error %s (%d): %s\n", str, e, strerror( e ) );
}

int main( int argc, char** argv )
{
    char debug_filename[] = "debug_cmd_XXXXXX";
    init_debug( debug_filename );

    const char* filename = argc >= 2 ? argv[ 1 ] : OUTPUT_SOCKET;

    struct sockaddr_un addr = { .sun_family = AF_UNIX, };

    if ( snprintf( addr.sun_path, sizeof addr.sun_path, "%s", filename )
            >= ( int )sizeof addr.sun_path )
        return fprintf( stderr, "socket filename too long\n" );

    int sock_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
    if ( sock_fd == -1 )
        return cerror( "socket" ), 1;

    if ( connect( sock_fd, ( struct sockaddr* )&addr, sizeof addr ) == -1 )
        // TODO: Doesn't really matter, but perhaps be cleaner about closing
        // things, or use ‹err› or similar.
        return cerror( "connect" ), 1;

    char buf[ 128 ];
    int bytes;

    // Echo input from stdin to the socket.
    while ( ( bytes = read( 0, buf, sizeof buf ) ) > 0 )
    {
        dprintf( 3, "cmd: read \"%.*s\"\n", bytes, buf );

        int r = write( sock_fd, buf, bytes );
        if ( r == -1 )
            return cerror( "write" ), 1;

        if ( r != bytes )
            return fprintf( stderr, "written less\n" ), 1;
    }

    close( sock_fd );

    return 0;
}
