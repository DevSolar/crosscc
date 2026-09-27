#ifndef NDEBUG
#define SOLOG_IMPLEMENTATION ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR | SOLOG_FEATURE_FILE | SOLOG_FEATURE_LINE )
#define SOLOG_SZ_FILE 12
#define SOLOG_SZ_LINE 6
#else
#define SOLOG_IMPLEMENTATION
#endif
#include "support/solog.h"

int main( void )
{
    SOLOG( INFO, "CrossCC started." );
}
