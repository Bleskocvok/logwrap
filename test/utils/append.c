#define _POSIX_C_SOURCE 200809L

#include <sys/socket.h>     // sockaddr_un, socket, connect
#include <sys/un.h>         // sockaddr_un
#include <unistd.h>         // close, read, write
#include <sys/stat.h>       // open
#include <fcntl.h>          // open
#include <stdio.h>          // dprintf
#include <errno.h>          // errno
#include <string.h>         // strerror
#include <time.h>           // nanosleep

#include <stdio.h>          // perror, snprintf
#include <stdlib.h>         // exit

void cerror( const char* str )
{
    int e = errno;
    fprintf( stderr, "%s (%d): %s\n", str, e, strerror( e ) );
    fflush( stderr );
    dprintf( 3, "append: error %s (%d): %s\n", str, e, strerror( e ) );
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

int main( int argc, char** argv )
{
    if ( argc < 2 )
        return 1;

    const char* filename = argv[ 1 ];

    pid_t pid = getpid();
    srand( pid );
    ms_sleep( rand() % 500 + 200 );

    char buf[ 128 ];
    int bytes;

    int fd = open( filename, O_WRONLY | O_APPEND );

    // Append input from stdin to the file.
    while ( ( bytes = read( 0, buf, sizeof buf ) ) > 0 )
    {
        int r = write( fd, buf, bytes );
        if ( r == -1 )
            return cerror( "write" ), 1;

        if ( r != bytes )
            return fprintf( stderr, "written less\n" ), 1;
    }

    close( fd );

    return 0;
}
