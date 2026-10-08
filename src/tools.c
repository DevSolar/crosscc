#include "tools.h"

#include "support/solog.h"

#include <string.h>

void tool_flags( vec( string ) * options, tool_flag_t * flags )
{
    SOLOG( TRACE, "tool_flags( options=%p, flags=%p )", (void *)options, (void *)flags );

    if ( options == NULL || flags == NULL )
    {
        SOLOG( WARN, "tool_flags called with NULL pointer (options=%p, flags=%p)", (void *)options, (void *)flags );
        return;
    }

    cc_foreach( options, opt )
    {
        char const * opt_str = first( opt );

        if ( opt_str == NULL )
        {
            continue;
        }

        for ( tool_flag_t * f = flags; f->flag != NULL; ++f )
        {
            if ( strcmp( opt_str, f->flag ) == 0 )
            {
                size_t idx = (size_t)( opt - first( options ) );
                SOLOG( DEBUG, "tool_flags matched flag '%s'", f->flag ? f->flag : "(null)" );
                f->is_set = true;
                erase( options, idx );
                --opt;
                break;
            }
        }
    }
}
