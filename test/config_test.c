#include "support/greatest.h"
#include "config.h"

TEST test_dummy( void )
{
    PASS();
}

SUITE( config_suite )
{
    RUN_TEST( test_dummy );
}
