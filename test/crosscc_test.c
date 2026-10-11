#include "support/greatest.h"

#define SOLOG_IMPLEMENTATION ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR | SOLOG_FEATURE_FILE | SOLOG_FEATURE_LINE )
#define SOLOG_SZ_FILE 12
#define SOLOG_SZ_LINE 6
#include "support/solog.h"

#include <stdio.h>

#ifndef _WIN32
#define DEV_NULL "/dev/null"
#else
#define DEV_NULL "NUL"
#endif

SUITE_EXTERN( utils_suite );
SUITE_EXTERN( context_suite );
SUITE_EXTERN( options_suite );
SUITE_EXTERN( tools_suite );

GREATEST_MAIN_DEFS();

int main( int argc, char * argv[] )
{
    if ( freopen( DEV_NULL, "w", stdout ) == NULL )
    {
        /* Continue even if stdout suppression fails */
    }
    GREATEST_MAIN_BEGIN();
    solog_config.level = SOLOG_LVL_FAIL;
    RUN_SUITE( utils_suite );
    RUN_SUITE( context_suite );
    RUN_SUITE( options_suite );
    RUN_SUITE( tools_suite );
    GREATEST_MAIN_END();
}
