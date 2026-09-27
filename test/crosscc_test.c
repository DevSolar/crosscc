#include "support/greatest.h"

#define SOLOG_IMPLEMENTATION ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR | SOLOG_FEATURE_FILE | SOLOG_FEATURE_LINE )
#define SOLOG_SZ_FILE 12
#define SOLOG_SZ_LINE 6
#include "support/solog.h"

SUITE_EXTERN( utils_suite );

GREATEST_MAIN_DEFS();

int main( int argc, char * argv[] )
{
    GREATEST_MAIN_BEGIN();
    solog_config.level = SOLOG_LVL_FAIL;
    RUN_SUITE( utils_suite );
    GREATEST_MAIN_END();
}
