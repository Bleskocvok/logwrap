#include <stdio.h>      // fprintf
#include <stdlib.h>     // strtol

int main( int argc, char** argv )
{
    if ( argc < 2 )
        return fprintf( stderr, "usage: %s exit_code\n", argv[ 0 ] );

    return strtol( argv[ 1 ], 0, 0 );
}
