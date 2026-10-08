#ifndef CROSSCC_TOOLS_H
#define CROSSCC_TOOLS_H

#include "support/cc_include.h"

#include <stdbool.h>

bool concat( vec( string ) * options );
bool copy( vec( string ) * options );
bool copydir( vec( string ) * options );
bool echo( vec( string ) * options );
bool unset_env( vec( string ) * options );
bool set_env( vec( string ) * options );
bool makedir( vec( string ) * options );
bool checksum( vec( string ) * options );
bool rm( vec( string ) * options );
bool sleep( vec( string ) * options );
bool touch( vec( string ) * options );
bool link( vec( string ) * options );
bool unpack( vec( string ) * options );

#endif
