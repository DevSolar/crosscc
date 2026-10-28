#ifndef NDEBUG
#define SOLOG_IMPLEMENTATION ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR | SOLOG_FEATURE_FILE | SOLOG_FEATURE_LINE )
#define SOLOG_SZ_FILE 16
#define SOLOG_SZ_LINE 6
#else
#define SOLOG_IMPLEMENTATION
#endif
#include "support/solog.h"

#include "context.h"
#include "options.h"
#include "config.h"

int main( int argc, char * argv[] )
{
    ctx_init();
    parse_options( argc, argv );

    switch ( g_ctx.verbose )
    {
        case 0:
            SOLOG( TRACE, "Setting ERR verbosity" );
            solog_config.level = SOLOG_LVL_ERR;
            break;
        case 2:
            SOLOG( TRACE, "Setting DEBUG verbosity" );
            solog_config.level = SOLOG_LVL_DEBUG;
            break;
        case 99:
            SOLOG( TRACE, "Setting TRACE verbosity" );
            solog_config.level = SOLOG_LVL_TRACE;
            break;
        default:
            SOLOG( TRACE, "Setting INFO verbosity" );
            solog_config.level = SOLOG_LVL_INFO;
            break;
    }

    if ( g_ctx.help )
    {
        help();
    }
    else if ( g_ctx.info )
    {
        list_targets();
    }
    else
    {
        load_cfg();
    }

    ctx_cleanup();
}
