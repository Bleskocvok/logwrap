#define _POSIX_C_SOURCE 200809L

#include <unistd.h>         // fork

#include <stdint.h>         // uint32_t

#define PROG_PATH "../logwrap"
#define TEST_SERVER "utils/server"
#define TEST_CMD "utils/cmd"
#define TEST_EXIT "utils/exit"
#define TEST_SIGNAL "utils/signal"
#define TEST_APPEND "utils/append"

#define SIZE( x ) ( sizeof x / sizeof *x )

typedef struct
{
    int in;
    int out;

} link_t;

typedef struct
{
    const char* data[ 64 ];
    int size;

} args_t;

typedef struct
{
    char CMD_OUT[ 128 ];
    char CMD_ERR[ 128 ];
    char SERVER_OUT[ 128 ];
    char SERVER_ERR[ 128 ];

    args_t args;

    link_t sout;
    link_t ser;

    pid_t pid;

} config_t;

int start_server( const char* cmd_output_socket );
int start_connection( const char* server_input );
void error( const char* str );
void assert_put( link_t lnk, const char* str );
void assert_get( link_t lnk, const char* expected );
void assert_timeout_get( link_t lnk, int timeout_ms );
pid_t fork_exec( const char* cmd, const char* argv[] );
void ms_sleep( int ms );

char* make_random_long( uint32_t seed, unsigned len, int newline );

args_t new_args();
void args_push( args_t* args, const char* arg );
void args_append( args_t* args, const char* elems[], int size );

void config_init( config_t* conf, const char* co, const char* ce,
                                  const char* so, const char* se );
void begin( config_t* conf );
void end( config_t* conf );
