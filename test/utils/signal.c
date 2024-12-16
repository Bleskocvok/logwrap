#define _POSIX_C_SOURCE 200809L

#include <signal.h>     // raise
#include <stdlib.h>     // strtoul
#include <stdio.h>      // printf

int main( int argc, char** argv )
{
    if ( argc < 2 )
        return fprintf( stderr, "usage: %s signal\n", argv[ 0 ] ), 1;

    int sig = strtoul( argv[ 1 ], 0, 0 );

    printf("raise(%d)\n", sig);

    if ( raise( sig ) != 0 )
        return 1;
    return 0;
}
