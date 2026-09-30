#include "options.h"

#include "context.h"
#include "utils.h"

#include "support/solog.h"

#include <ctype.h>

static void parse_bool( bool * dest, strspan_t span );
static void parse_long( long * dest, strspan_t span );
static void parse_string( string * dest, strspan_t span );

static bool parse_option( string * option, void * unused );

void parse_options( int argc, char * argv[] )
{
    /* Cannot rely on ready-made argv parsers like parg.h, as we need
     * to funnel all CrossCC options through -Wx,... and ignore all
     * other options -- generic argv parsers don't like that, so we
     * need to roll our own.
     */
    int i;

    if ( argc <= 1 || argv == NULL )
    {
        SOLOG( ERR, "No command line arguments to parse." );
        return;
    }

    SOLOG( TRACE, "Parsing %d command line options", argc - 1 );

    for ( i = 1; i < argc; ++i )
    {
        if ( argv[i] == NULL )
        {
            SOLOG( WARN, "Command line option %d is (null)", i );
            continue;
        }

        if ( strncmp( argv[i], "-Wx,", 4 ) == 0 )
        {
            SOLOG( TRACE, "'%s': CrossCC option", argv[i] );
            if ( foreach( argv[i] + 4, ",", parse_option, NULL ) <= 0 )
            {
                SOLOG( ERR, "Failed to parse options from '%s'", argv[i] );
                exit( EXIT_FAILURE );
            }
        }
        else
        {
            string arg;
            init( &arg );
            SOLOG( TRACE, "'%s': Passthrough option", argv[i] );
            push_fmt( &arg, argv[i] );
            push( &g_ctx.passthrough, arg );
        }
    }
}

static void parse_bool( bool * dest, strspan_t span )
{
    size_t len;

    if ( ! dest )
    {
        return;
    }

    if ( ! span.begin || span.begin >= span.end )
    {
        *dest = true;
        return;
    }

    len = (size_t)( span.end - span.begin );

    if ( ( len == 4 && strnicmp( span.begin, "true", 4 ) == 0 ) ||
         ( len == 1 && strnicmp( span.begin, "1", 1 ) == 0 ) ||
         ( len == 3 && strnicmp( span.begin, "yes", 3 ) == 0 ) ||
         ( len == 2 && strnicmp( span.begin, "on", 2 ) == 0 ) )
    {
        *dest = true;
    }
    else if ( ( len == 5 && strnicmp( span.begin, "false", 5 ) == 0 ) ||
              ( len == 1 && strnicmp( span.begin, "0", 1 ) == 0 ) ||
              ( len == 2 && strnicmp( span.begin, "no", 2 ) == 0 ) ||
              ( len == 3 && strnicmp( span.begin, "off", 3 ) == 0 ) )
    {
        *dest = false;
    }
    else
    {
        SOLOG( WARN, "Unrecognized boolean value '%.*s', defaulting to true", (int)len, span.begin );
        *dest = true;
    }
}

/* While using 'long' as type, the only currently supported option is
 * verbosity, which accepts single-digit values only. Therefore we do
 * not do a proper string-to-long here, but consider anything beyond
 * one digit an error.
 */
static void parse_long( long * dest, strspan_t span )
{
    char const * p = span.begin;
    *dest = 0;

    while ( p < span.end )
    {
        if ( isdigit( (unsigned char)*p ) )
        {
            *dest = *dest * 10 + ( *p - '0' );
            ++p;
        }
        else
        {
            SOLOG( WARN, "Option value '%.*s' not numerical", (size_t)( span.end - span.begin ), span.begin );
            return;
        }
    }
}

static void parse_string( string * dest, strspan_t span )
{
    clear( dest );
    push_n( dest, span.begin, (size_t)( span.end - span.begin ) );
}

static bool parse_option( string * option, void * unused )
{
    strspan_t span;
    (void)unused;

    if ( option == NULL || size( option ) == 0 || first( option ) == NULL )
    {
        SOLOG( ERR, "Option is NULL / empty" );
        return false;
    }

    switch ( *first( option ) )
    {
#define X( name, initial, type, desc ) \
        case initial: \
            if ( strncmp( first( option ), #name, sizeof( #name ) - 1 ) || \
                    ( size( option ) > sizeof( #name ) - 1 && \
                      *( first( option ) + sizeof( #name ) - 1 ) != '=' ) ) \
            { \
                span.begin = first( option ) + 1; \
                if ( size( option ) > 1 && *( first( option ) + 1 ) == '=' ) \
                { \
                    ++span.begin; \
                } \
            } \
            else \
            { \
                span.begin = first( option ) + sizeof( #name ) - 1; \
                if ( span.begin < end( option ) && *span.begin == '=' ) \
                { \
                    ++span.begin; \
                } \
            } \
            span.end = end( option ); \
            parse_ ## type( &g_ctx.name, span ); \
            break;
        OPTIONS( X )
#undef X
        default:
            SOLOG( ERR, "Option '%s' invalid", first( option ) );
            return false;
    }

    return true;
}
