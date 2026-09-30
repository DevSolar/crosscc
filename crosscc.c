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

int main( int argc, char * argv[] )
{
    solog_config.level = SOLOG_LVL_TRACE;
    ctx_init();
    parse_options( argc, argv );
    ctx_cleanup();
}
