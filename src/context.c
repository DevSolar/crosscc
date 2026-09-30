#include "context.h"

#include "support/solog.h"

context_t g_ctx = { 0 };
static bool g_ctx_initialized = false;

void ctx_init( void )
{
    if ( g_ctx_initialized )
    {
        return;
    }

    g_ctx_initialized = true;

#define INIT_string( name ) init( &g_ctx.name );
#define INIT_bool( name ) g_ctx.name = false;
#define INIT_long( name ) g_ctx.name = 0;
#define X( name, initial, type, desc ) INIT_ ## type( name )
    OPTIONS( X )
#undef X
#undef INIT_long
#undef INIT_bool
#undef INIT_string

    init( &g_ctx.passthrough );
    init( &g_ctx.settings );

    atexit( ctx_cleanup );
}

void ctx_cleanup( void )
{
    if ( ! g_ctx_initialized )
    {
        return;
    }

    g_ctx_initialized = false;

#define CLEANUP_string( name ) cleanup( &g_ctx.name );
#define CLEANUP_bool( name ) g_ctx.name = false;
#define CLEANUP_long( name ) g_ctx.name = 0;
#define X( name, initial, type, desc ) CLEANUP_ ## type( name )
    OPTIONS( X )
#undef X
#undef CLEANUP_long
#undef CLEANUP_bool
#undef CLEANUP_string

    cleanup( &g_ctx.passthrough );
    cleanup( &g_ctx.settings );
}

void dump( void )
{
#define FMT_string "'%s'"
#define FMT_bool "%s"
#define FMT_long "%ld"
#define VAL_string( name ) first( &g_ctx.name )
#define VAL_bool( name ) ( g_ctx.name ? "true" : "false" )
#define VAL_long( name ) g_ctx.name
#define X( name, initial, type, desc ) SOLOG( DEBUG, "ctx." #name " = " FMT_ ## type, VAL_ ## type( name ) );
    OPTIONS( X )
#undef X
#undef VAL_long
#undef VAL_bool
#undef VAL_string
#undef FMT_long
#undef FMT_bool
#undef FMT_string
}

void help( void )
{
    puts( "Usage: crosscc [crosscc options] [passthrough arguments...]" );
    puts( "Options (passed via -Wx,<opt>[=<val>]):" );
#define X( name, initial, type, desc ) \
    printf( "  %c[%s]%*s%s\n", initial, &(#name[1]), (int)( 11 - sizeof( #name ) ), "", desc );
    OPTIONS( X )
#undef X
}
