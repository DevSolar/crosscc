#ifndef CROSSCC_TOOLS_H
#define CROSSCC_TOOLS_H

#include "support/cc_include.h"

#include <stdbool.h>

bool concat( vec( string ) );
bool chdir( vec( string ) );
bool is_different( vec( string ) );
bool copy( vec( string ) );
bool copydir( vec( string ) );
bool echo( vec( string ) );
bool unset_env( vec( string ) );
bool set_env( vec( string ) );
bool makedir( vec( string ) );
bool checksum( vec( string ) );
bool delete( vec( string ) );
bool sleep( vec( string ) );
bool touch( vec( string ) );
bool link( vec( string ) );
bool unpack( vec( string ) );

/* Internal use */

typedef struct
{
    char const * flag;
    bool is_set;
} tool_flag_t;

void tool_flags( vec( string ) * options, tool_flag_t * flags );

#endif
