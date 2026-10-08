#include "tools.h"

#include "support/solog.h"
#include "utils.h"

#include <string.h>

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

typedef struct
{
    char const * flag;
    bool is_set;
} tool_flag_t;

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif

static void extract_flags( vec( string ) * options, tool_flag_t * flags )
{
    size_t i;
    string * opt;
    char const * opt_str;
    bool matched;
    tool_flag_t * f;

    SOLOG( TRACE, "extract_flags( options=%p, flags=%p )", (void *)options, (void *)flags );

    if ( options == NULL || flags == NULL )
    {
        SOLOG( WARN, "extract_flags() called with NULL pointer (options=%p, flags=%p)", (void *)options, (void *)flags );
        return;
    }

    i = 0;
    while ( i < size( options ) )
    {
        opt = get( options, i );
        opt_str = first( opt );
        matched = false;

        if ( opt_str != NULL )
        {
            for ( f = flags; f->flag != NULL; ++f )
            {
                if ( strcmp( opt_str, f->flag ) == 0 )
                {
                    SOLOG( DEBUG, "extract_flags() matched flag '%s'", f->flag );
                    f->is_set = true;
                    erase( options, i );
                    matched = true;
                    break;
                }
            }
        }

        if ( ! matched )
        {
            ++i;
        }
    }
}

bool concat( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "concat" );
    }
    else
    {
        /* TODO: Print file contents to stdout */
    }

    return rc;
}

bool copy( vec( string ) * options )
{
    bool rc = false;
    string * src;
    string * dst;

    tool_flag_t flags[] =
    {
        { "--if-newer", false },
        { NULL, false }
    };

    extract_flags( options, flags );

    if ( size( options ) == 2 )
    {
        src = get( options, 0 );
        dst = get( options, 1 );

        if ( ! flags[0].is_set || is_newer( first( src ), first( dst ) ) )
        {
            /* TODO: Copy file */
        }
    }
    else
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "copy", 2, size( options ) );
    }

    return rc;
}

bool copydir( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 2 )
    {
        /* TODO: Copy directory */
    }
    else
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "copydir", 2, size( options ) );
    }

    return rc;
}

bool echo( vec( string ) * options )
{
    bool rc = false;
    (void)options;
    /* TODO: Print options to stdout */
    return rc;
}

bool unset_env( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "unset_env" );
    }
    else
    {
        /* TODO: Unset environment variable(s) */
    }

    return rc;
}

bool set_env( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 2 )
    {
        /* TODO: Set environment variable(s) */
    }
    else
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "set_env", 2, size( options ) );
    }

    return rc;
}

bool makedir( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "makedir" );
    }
    else
    {
        /* TODO: Create directory */
    }

    return rc;
}

bool checksum( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 1 )
    {
        /* TODO: Calculate checksum of file */
    }
    else
    {
        SOLOG( ERR, "'%s' expects 1 argument; %zu provided", "checksum", size( options ) );
    }

    return rc;
}

bool rm( vec( string ) * options )
{
    bool rc = false;

    tool_flag_t flags[] =
    {
        { "--force", false },
        { "--all", false },
        { NULL, false }
    };

    extract_flags( options, flags );

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "rm" );
    }
    else
    {
        /* TODO: remove / unlink files */
    }

    return rc;
}

bool sleep( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 1 )
    {
        /* TODO: Sleep for the given number of seconds */
    }
    else
    {
        SOLOG( ERR, "'%s' expects 1 argument; %zu provided", "sleep", size( options ) );
    }

    return rc;
}

bool touch( vec( string ) * options )
{
    bool rc = false;

    tool_flag_t flags[] =
    {
        { "--create", false },
        { NULL, false }
    };

    extract_flags( options, flags );

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "touch" );
    }
    else
    {
        /* TODO: Update files modification date;
         * optionally create them if not already existing
         */
    }

    return rc;
}

bool link( vec( string ) * options )
{
    bool rc = false;

    if ( size( options ) == 2 )
    {
        /* TODO: Create file #1 as link to file #2 */
    }
    else
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "link", 2, size( options ) );
    }

    return rc;
}

bool unpack( vec( string ) * options )
{
    bool rc = false;
    (void)options;
    /* TODO: NO idea how to provide this in a generic manner */
    return rc;
}
