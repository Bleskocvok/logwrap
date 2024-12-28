#define _POSIX_C_SOURCE 200809L

#include <unistd.h>     // write, read, close, fork, exec, pipe, dup2
#include <sys/wait.h>   // waitpid
#include <sys/types.h>  // pid_t
#include <poll.h>       // poll
#include <time.h>       // clock_gettime

#include <stdio.h>      // printf, dprintf, perror
#include <stdlib.h>     // NULL, malloc, free, exit, strtod
#include <string.h>     // memset, memcpy, strcmp
#include <signal.h>     // signal
#include <assert.h>     // assert


static int DELAY_MSEC = -1;
static int DETACH = 0;
static int JOIN = 0;
static int PREFIX = 0;
static int ENSURE_NEWLINE = 0;


typedef struct buf_t
{
    char* data;
    char* aux_data;
    unsigned int size;
    unsigned int cap;

    struct timespec time_begin;
    struct timespec time_unfinished;
    struct timespec time_last;

    int newline_present;

} buf_t;


typedef struct pid_vec_t
{
    pid_t pids[ 1024 ];
    unsigned int size;

} pid_vec_t;


typedef struct prog_t
{
    char* cmd;
    char** args;

} prog_t;


void print_args( prog_t prog );

typedef struct output_t
{
    prog_t prog;

    pid_vec_t pids;

} output_t;


struct timespec get_time()
{
    struct timespec ts;
    if ( clock_gettime( CLOCK_MONOTONIC, &ts ) == -1 )
        perror( "clock_gettime" ), exit( 1 );
    return ts;
}


void pid_vec_init( pid_vec_t* vec )
{
    memset( vec, 0, sizeof( pid_vec_t ) );
}


void pid_vec_try_reap( pid_vec_t* vec )
{
    char remove[ sizeof( vec->pids ) / sizeof( vec->pids[ 0 ] ) ] = { 0 };

    for ( unsigned i = 0; i < vec->size; ++i )
    {
        int status;
        int r = waitpid( vec->pids[ i ], &status, WNOHANG );
        if ( r == -1 )
        {
            perror( "waitpid" );
            continue;
        }

        if ( WIFEXITED( status ) && WEXITSTATUS( status ) != 0 )
            dprintf( 2, "warning: exit status %d\n", WEXITSTATUS( status ) );
        if ( WIFSIGNALED( status ) )
            dprintf( 2, "warning: signaled %d\n", WTERMSIG( status ) );

        remove[ i ] = r > 0;
    }

    int last = 0;
    for ( unsigned i = 0; i < vec->size; ++i )
    {
        if ( remove[ i ] ) continue;

        vec->pids[ last ] = vec->pids[ i ];
        ++last;
    }

    vec->size = last;
}


void pid_vec_push( pid_vec_t* vec, pid_t pid )
{
    // This could be solved in a much better way which would not be relying on
    // the executed programs to finish within a reasonable time frame. One
    // possible approach is catching SIGCHLD to see when a child has died.
    while ( vec->size >= sizeof( vec->pids ) / sizeof( vec->pids[ 0 ] ) )
        pid_vec_try_reap( vec );

    vec->pids[ vec->size ] = pid;
    ++vec->size;
}


void pid_vec_wait( pid_vec_t* vec )
{
    for ( unsigned i = 0; i < vec->size; ++i )
    {
        int status;
        if ( waitpid( vec->pids[ i ], &status, 0 ) == -1 )
            perror( "waitpid" );

        if ( WIFEXITED( status ) && WEXITSTATUS( status ) != 0 )
            dprintf( 2, "warning: exit status %d\n", WEXITSTATUS( status ) );
        if ( WIFSIGNALED( status ) )
            dprintf( 2, "warning: signaled %d\n", WTERMSIG( status ) );
    }

    pid_vec_init( vec );
}


void output_init( output_t* o )
{
    memset( o, 0, sizeof( output_t ) );
}


unsigned buf_aux_size( const buf_t* b )
{
    return b->cap * 2;
}


void buf_init( buf_t* b )
{
    memset( b, 0, sizeof( buf_t ) );
    b->time_begin      = get_time();
    b->time_unfinished = b->time_begin;
    b->time_last       = b->time_begin;

    b->size = 0;
    b->cap = 256;
    if ( !( b->data = malloc( b->cap ) )
      || !( b->aux_data = malloc( 2 * b->cap ) ) )
        perror( "malloc" ), exit( 1 );
}


void buf_free( buf_t* b )
{
    free( b->data );
    free( b->aux_data );
}


void buf_push( buf_t* b, char c )
{
    if ( b->size + 1 >= b->cap )
    {
        void* ptr = NULL;
        void* aux_ptr = NULL;
        if ( !( ptr = realloc( b->data, 2 * b->cap ) )
          || !( aux_ptr = realloc( b->aux_data, 4 * b->cap ) ) )
        {
            free( ptr );
            free( aux_ptr );
            perror( "realloc" );
            return;
        }
        b->data = ptr;
        b->aux_data = aux_ptr;
        b->cap *= 2;
    }

    b->data[ b->size ] = c;
    ++b->size;
}


void buf_append( buf_t* b, const char* str, int len )
{
    struct timespec ts = get_time();
    if ( b->size == 0 )
        b->time_begin = ts;

    b->time_last = ts;

    for ( int i = 0; i < len; ++i )
    {
        if ( str[ i ] == '\n' )
        {
            b->newline_present = 1;
            // I think this is correct, even if the code doesn't care
            // whether an unfinished line begins after this character.
            b->time_unfinished = ts;
        }
        buf_push( b, str[ i ] );
    }
}


int buf_size( const buf_t* b ) { return b->size; }


int fork_exec_out( prog_t prog, const char* output, int length );


void output_flush_cmd( output_t* output, const char* str, int len )
{
    if ( DETACH )
    {
        pid_t pid = fork();
        if ( pid != -1 )
        {
            // Note: double ‹fork› is used because of blocking ‹write›. It runs
            // in a child process whose child is ‹exec›ing.
            if ( pid == 0 )
                exit( fork_exec_out( output->prog, str, len ) == -1 ? 1 : 0 );
            pid_vec_push( &output->pids, pid );
        }
        else
            perror( "fork" );
    }
    else
        fork_exec_out( output->prog, str, len );
}


void buf_flush( buf_t* b, output_t* output, const char* prefix, int ignore_newline )
{
    if ( buf_size( b ) == 0 )
        return;

    unsigned end = 0;
    for ( unsigned i = 0; i < b->size; ++i )
    {
        if ( b->data[ i ] == '\n' )
        {
            end = i + 1;
            // TODO: Refactor this.
            if ( DELAY_MSEC < 0 )
                break;
        }
    }
    if ( ignore_newline )
        end = b->size;

    assert( end > 0 );

    if ( b->data[ end - 1 ] == '\n' || !ENSURE_NEWLINE )
    {
        snprintf( b->aux_data, buf_aux_size( b ), "%s%.*s", prefix, end, b->data );
    }
    else
    {
        snprintf( b->aux_data, buf_aux_size( b ), "%s%.*s\n", prefix, end, b->data );
        // For the added newline.
        // end++;
    }

    output_flush_cmd( output, b->aux_data, strlen( b->aux_data ) );

    int new_size = b->size - end;
    if ( new_size < 0 )
        new_size = 0;
    if ( end < b->size )
        memmove( b->data, b->data + end, new_size );

    b->data[ new_size ] = 0;

    b->size = new_size;
    b->time_begin = b->time_unfinished;
    b->newline_present = memchr( b->data, '\n', b->size ) != NULL;

    b->time_begin = new_size > 0 ? b->time_unfinished : get_time();
}


void buf_try_flush( buf_t* b, output_t* o, int thresh_ms, const char* prefix )
{
    if ( !b->newline_present )
        return;

    do
    {
    struct timespec now = get_time();

    // TODO: This sucks, use a better way to do arithmetics with timespec.
    double diff =   now.tv_sec  - b->time_begin.tv_sec
                + ( now.tv_nsec - b->time_begin.tv_nsec ) / 1e9;
    diff *= 1e3;
    if ( thresh_ms < 0 || diff >= thresh_ms )
        buf_flush( b, o, prefix, 0 );
    else
        break;

    } while ( b->newline_present );
}


int buf_elapsed_ms( buf_t* b, struct timespec now )
{
    if ( !b->newline_present || b->size == 0 )
        return -1;

    double diff =   now.tv_sec  - b->time_begin.tv_sec
                + ( now.tv_nsec - b->time_begin.tv_nsec ) / 1e9;
    diff *= 1e3;
    return diff;
}


int fork_exec_out( prog_t prog, const char* output, int length )
{
    int p_out[ 2 ];
    if ( pipe( p_out ) == -1 )
        return -1;

    pid_t pid = fork();
    if ( pid == -1 ) return perror( "fork" ), -1;

    if ( pid > 0 )
    {
        close( p_out[ 0 ] );

        if ( write( p_out[ 1 ], output, length ) == -1 )
            perror( "write" );

        close( p_out[ 1 ] );

        if ( waitpid( pid, NULL, 0 ) == -1 )
            perror( "waitpid" );

        return pid;
    }

    close( p_out[ 1 ] );
    if ( dup2( p_out[ 0 ], STDIN_FILENO ) == -1 )
        exit( 100 );
    close( p_out[ 0 ] );

    execvp( prog.cmd, prog.args );
    exit( 100 );
}


int process( int fd, buf_t* buf )
{
    char tmp[ 256 ] = { 0 };

    int bytes;
    if ( ( bytes = read( fd, tmp, sizeof tmp ) ) == -1 )
        return -1;

    buf_append( buf, tmp, bytes );

    return bytes;
}


int parent( pid_t child, int fd_child_out, int fd_child_err, int* status,
            output_t outputs[ 2 ] )
{
    struct pollfd polled[ 2 ] = { { .fd = fd_child_out, .events = POLLIN, },
                                  { .fd = fd_child_err, .events = POLLIN, }, };

    const char* prefs[ 2 ] = { "STDOUT: ", "STDERR: " };
    buf_t stor[ 2 ];
    buf_init( &stor[ 0 ] );
    buf_init( &stor[ 1 ] );

    buf_t* bufs[ 2 ] = { &stor[ 0 ], &stor[ 1 ] };
    output_t* outs[ 2 ] = { &outputs[ 0 ], &outputs[ 1 ] };

    if ( JOIN )
    {
        outs[ 1 ] = outputs;
        bufs[ 1 ] = stor;
    }

    if ( !PREFIX )
        prefs[ 1 ] = prefs[ 0 ] = "";

    while ( polled[ 0 ].fd != -1 || polled[ 1 ].fd != -1 )
    {
        // TODO: Refactor this whole chunk.
        // At least 1s timeout.
        int wait_ms = 1000;
        if ( DELAY_MSEC > 0 )
        {
            struct timespec now = get_time();
            int a = buf_elapsed_ms( bufs[ 0 ], now );
            int b = buf_elapsed_ms( bufs[ 1 ], now );
            a = a < 0 ? wait_ms : DELAY_MSEC - a;
            b = b < 0 ? wait_ms : DELAY_MSEC - b;
#define wmin( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )
            wait_ms = wmin( wait_ms, wmin( a, b ) );
#undef wmin
        }

        int p = poll( polled, 2, wait_ms );
        if ( p == -1 ) perror( "polling" );

        for ( int i = 0; i < 2; i++ )
        {
            int ended = 0;

            if ( polled[ i ].fd == -1 )
                continue;

            if ( ( polled[ i ].revents & ( POLLIN | POLLHUP ) ) )
                if ( process( polled[ i ].fd, bufs[ i ] ) == 0 )
                    ended = 1;

            if ( ( polled[ i ].revents & POLLERR ) == POLLERR )
            {
                polled[ i ].fd = -1;
                dprintf( 1, "%d: error\n", i );
            }

            if ( ended || polled[ i ].fd == -1 )
            {
                buf_flush( bufs[ i ], outs[ i ], prefs[ i ], 1 );
                polled[ i ].fd = -1;
            }
        }

        for ( int i = 0; i < 2; ++i )
        {
            buf_try_flush( bufs[ i ], outs[ i ], DELAY_MSEC, prefs[ i ] );

            pid_vec_try_reap( &outs[ i ]->pids );
        }
    }

    // flush unterminated lines
    for ( int i = 0; i < 2; ++i )
    {
        if ( polled[ i ].fd == -1 || bufs[ i ]->size == 0 )
            continue;

        if ( buf_size( bufs[ i ] ) > 0 )
            buf_flush( bufs[ i ], outs[ i ], prefs[ i ], 1 );
    }

    close( fd_child_out );
    close( fd_child_err );

    buf_free( &stor[ 0 ] );
    buf_free( &stor[ 1 ] );

    if ( waitpid( child, status, 0 ) == -1 )
        return perror( "waitpid" ), -1;
    return 0;
}


int wrap( prog_t prog, output_t outputs[ 2 ] )
{
    int p_out[ 2 ];
    int p_err[ 2 ];
    if ( pipe( p_out ) == -1 || pipe( p_err ) == -1 )
        return -1;

    pid_t pid = fork();
    if ( pid == -1 ) return perror( "fork" ), -1;

    if ( pid > 0 )
    {
        close( p_out[ 1 ] );
        close( p_err[ 1 ] );
        int status;
        int r = parent( pid, p_out[ 0 ], p_err[ 0 ], &status, outputs );

        // Wait for outstanding processes.
        for ( int i = 0; i < 2; ++i )
            pid_vec_wait( &outputs[ i ].pids );

        if ( r != -1 )
        {
            if ( WIFEXITED( status ) ) return WEXITSTATUS( status );
            if ( WIFSIGNALED( status ) ) return 100 + WTERMSIG( status );
        }
        return r == -1 ? 1 : 0;
    }

    close( p_out[ 0 ] );
    close( p_err[ 0 ] );
    if ( dup2( p_out[ 1 ], STDOUT_FILENO ) == -1
      || dup2( p_err[ 1 ], STDERR_FILENO ) == -1 )
        perror( "dup2" ), exit( 100 );
    close( p_out[ 1 ] );
    close( p_err[ 1 ] );

    execvp( prog.cmd, prog.args );
    fprintf( stderr, "exec( %s )\n", prog.cmd );
    perror( "exec" ), exit( 100 );
}


void help()
{
    printf( "\n"
            "  -h           help\n"
            "  -s  sec      group consecutive messages within given window of time\n"
            "  -j           join stdout and stderr\n"
            "  -d           detach executed cmd\n"
            "  -p           prefix \"stdout\" and \"stderr\"\n"
            "  -n           add newline at the end of output if missing\n"
            );
}


void usage( char** argv )
{
    printf( "usage: %s [-h] [-s sec] [-j] [-d] [-p] <cmd> [<out cmd>] [<err cmd>]\n",
            argv[ 0 ] );
}


void print_args( prog_t prog )
{
    printf( "cmd → %s\n", prog.cmd );
    for ( int i = 0; ; i++ )
    {
        printf( "args[ %d ] -> %s\n", i, prog.args[ i ] ? prog.args[ i ] : "NULL" );
        if ( !prog.args[ i ] )
            break;
    }
}


void parse_args( int argc, char* const* argv, prog_t progs[ 3 ] );


int main( int argc, char** argv )
{
    if ( argc < 2 )
        return usage( argv ), 1;

    while ( argc > 1 )
    {
        if ( strlen( argv[ 1 ] ) != 2 || argv[ 1 ][ 0 ] != '-'
                || strcmp( argv[ 1 ], "--" ) == 0 )
            break;

        char arg = argv[ 1 ][ 1 ];

        if ( arg == 'h' )
        {
            return usage( argv ), help(), 0;
        }
        if ( arg == 's' )
        {
            if ( argc < 3 ) return usage( argv ), 1;

            double sec = strtod( argv[ 2 ], NULL );
            DELAY_MSEC = sec * 1000;

            argc--;
            argv++;
        }
        else if ( arg == 'd' )
        {
            DETACH = 1;
        }
        else if ( arg == 'j' )
        {
            JOIN = 1;
        }
        else if ( arg == 'p' )
        {
            PREFIX = 1;
        }
        else if ( arg == 'n' )
        {
            ENSURE_NEWLINE = 1;
        }
        else
            return usage( argv ), 1;

        argc--;
        argv++;
    }

    output_t outputs[ 2 ];

    for ( int i = 0; i < 2; ++i )
        output_init( outputs + i );

    prog_t progs[ 3 ] = { 0 };

    parse_args( argc, argv, progs );

    outputs[ 0 ].prog = progs[ 1 ];
    outputs[ 1 ].prog = progs[ 2 ];

    // Prevent ‹write› in this process from being able to kill the whole thing.
    if ( signal( SIGPIPE, SIG_IGN ) == SIG_ERR )
        perror( "signal" );

    int rv = wrap( progs[ 0 ], outputs );

    for ( int i = 0; i < 3; ++i )
        free( progs[ i ].args );

    return rv;
}


void parse_args( int argc, char* const* argv, prog_t progs[ 3 ] )
{
    int seps = 0;
    for ( int i = 1; i < argc; i++ )
        if ( strcmp( argv[ i ], "--" ) == 0 )
        {
            seps++;
            break;
        }

    if ( seps )
    {
        int counts[ 3 ] = { 0 };
        int current = 0;

        for ( int i = 1; i < argc; i++ )
        {
            if ( strcmp( argv[ i ], "--" ) == 0 )
            {
                if ( i != 1 && current < 2 )
                    current++;
            }
            else
                counts[ current ]++;
        }

        for ( int i = 0; i < 3; i++ )
        {
            if ( counts[ i ] == 0 )
                counts[ i ] = 1;

            unsigned size = ( 1 + counts[ i ] ) * sizeof( char** );
            progs[ i ].args = ( char** )malloc( size );
            if ( !progs[ i ].args )
                perror( "malloc" ), exit( 101 );

            memset( progs[ i ].args, 0, size );
        }

        current = 0;
        int j = 0;
        for ( int i = 1; i < argc; i++ )
        {
            if ( strcmp( argv[ i ], "--" ) == 0 )
            {
                if ( i != 1 && current < 2 )
                {
                    current++;
                    j = 0;
                }
            }
            else
            {
                progs[ current ].args[ j ] = argv[ i ];
                j++;
            }
        }

        for ( int i = 1; i < 3; i++ )
            if ( progs[ i ].args[ 0 ] == NULL )
                progs[ i ].args[ 0 ] = "cat";

        for ( int i = 0; i < 3; i++ )
        {
            progs[ i ].args[ counts[ i ] ] = NULL;
            progs[ i ].cmd = progs[ i ].args[ 0 ];
        }

        return;
    }
    else
    {
        char* args[ 3 ];
        args[ 0 ] = argv[ 1 ];
        args[ 1 ] = argc > 2 ? argv[ 2 ] : "cat";
        args[ 2 ] = argc > 3 ? argv[ 3 ] : "cat";

        for ( int i = 0; i < 3; i++ )
        {
            progs[ i ].cmd = args[ i ];
            progs[ i ].args = malloc( 2 * sizeof( *progs[ i ].args ) );
            if ( !progs[ i ].args )
                perror( "malloc" ), exit( 101 );
            progs[ i ].args[ 0 ] = progs[ i ].cmd;
            progs[ i ].args[ 1 ] = NULL;
        }

        return;
    }
}
