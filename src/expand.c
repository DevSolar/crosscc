#include "expand.h"

#include "utils.h"
#include "support/solog.h"

#include <stdarg.h>
#include <stdlib.h>

void expand_args( string * input, strspan_t * span, va_list ap );
void expand_env( string * input, strspan_t * span );
void expand_latest( string * input, strspan_t * span );
void expand_default( string * input, strspan_t * span );
void replace_span( string * input, strspan_t * span, char const * rep, size_t rep_len );

#define MACRO_TYPES(X)        \
    X( STR_ARGS, "ARGS:" )    \
    X( STR_ENV, "ENV:" )      \
    X( STR_LATEST, "LATEST" )

typedef enum
{
    STR_NO_MACRO = 0,
    STR_DEFAULT,
#define AS_ENUM( name, prefix ) name,
    MACRO_TYPES( AS_ENUM )
#undef AS_ENUM
} macro_type_t;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
typedef struct
{
    const char * prefix;
    size_t len;
    macro_type_t type;
} macro_type_map_t;
#pragma clang diagnostic pop

static const macro_type_map_t macro_type_map[] =
{
    { NULL, 0, STR_NO_MACRO },
    { NULL, 0, STR_DEFAULT },
#define AS_TABLE_ENTRY( name, prefix ) { prefix, sizeof( prefix ) - 1, name },
    MACRO_TYPES( AS_TABLE_ENTRY )
#undef AS_TABLE_ENTRY
};

#define macro_type_map_size ( sizeof( macro_type_map ) / sizeof( macro_type_map[0] ) )

static macro_type_t get_macro_type( strspan_t span )
{
    for ( size_t i = STR_DEFAULT + 1; i < macro_type_map_size; ++i )
    {
        if ( macro_type_map[i].prefix &&
             strncmp( span.begin + 1, macro_type_map[i].prefix, macro_type_map[i].len ) == 0 )
        {
            return macro_type_map[i].type;
        }
    }

    return STR_DEFAULT;
}

int expand( string * input, ... )
{
    strspan_t span = { NULL, first( input ) };

    while ( *span.end != '\0' )
    {
        macro_type_t macro_type = STR_NO_MACRO;

        switch ( *span.end )
        {
            case '{':
                span.begin = span.end;
                break;
            case '}':
                if ( span.begin != NULL )
                {
                    macro_type = get_macro_type( span );
                }
                break;
            default:
                break;
        }

        ++span.end;

        switch ( macro_type )
        {
            case STR_ENV:
                expand_env( input, &span );
                break;
            case STR_ARGS:
            {
                va_list ap;
                va_start( ap, input );
                expand_args( input, &span, ap );
                va_end( ap );
                break;
            }
            case STR_LATEST:
                expand_latest( input, &span );
                break;
            case STR_DEFAULT:
                expand_default( input, &span );
                break;
            case STR_NO_MACRO:
                break;
            default:
                SOLOG( ERR, "Internal error: Unimplemented macro type." );
                /* TODO */
                break;
        }
    }

    return 0;
}

/* Helper function actually doing the string replacement and span recalc */
void replace_span( string * input, strspan_t * span, char const * rep, size_t rep_len )
{
    size_t offset = (size_t)( span->begin - first( input ) );
    size_t span_len = (size_t)( span->end - span->begin );

    erase_n( input, offset, span_len );

    if ( rep != NULL && rep_len > 0 )
    {
        insert_n( input, offset, rep, rep_len );
    }

    span->begin = NULL;
    span->end = first( input ) + offset + ( rep != NULL ? rep_len : 0 );
}

void expand_args( string * input, strspan_t * span, va_list ap )
{
    char const * val = NULL;
    size_t const prefix_len = macro_type_map[ STR_ARGS ].len;

    if ( ! input || ! span || ! span->begin || ! span->end || span->begin >= span->end )
    {
        return;
    }

    if ( span->end > span->begin + 1 + prefix_len + 1 )
    {
        char * endp = NULL;
        long arg_num = strtol( span->begin + 1 + prefix_len, &endp, 10 );

        if ( arg_num > 0 && endp == span->end - 1 )
        {
            long i;

            for ( i = 1; i <= arg_num; ++i )
            {
                val = va_arg( ap, char const * );
            }

            SOLOG( TRACE, "Expanded {ARGS:%ld} -> '%s'", arg_num, val ? val : "(null)" );
        }
    }

    replace_span( input, span, val, val ? strlen( val ) : 0 );
}

void expand_env( string * input, strspan_t * span )
{
    char const * val = NULL;
    size_t const prefix_len = macro_type_map[ STR_ENV ].len;

    if ( ! input || ! span || ! span->begin || ! span->end || span->begin >= span->end )
    {
        return;
    }

    if ( span->end > span->begin + 1 + prefix_len + 1 )
    {
        char const * var_start = span->begin + 1 + prefix_len;
        size_t const var_len = (size_t)( (span->end - 1) - var_start );

        string envvar;
        init( &envvar );
        push_n( &envvar, var_start, var_len );

        if ( strchr( first( &envvar ), '=' ) == NULL )
        {
            val = getenv( first( &envvar ) );
        }

        SOLOG( TRACE, "Expanded {ENV:%s} -> '%s'", first( &envvar ), val ? val : "(null)" );
        cleanup( &envvar );
    }

    replace_span( input, span, val, val ? strlen( val ) : 0 );
}

void expand_latest( string * input, strspan_t * span )
{
    if ( ! input || ! span || ! span->begin || ! span->end || span->begin >= span->end )
    {
        return;
    }

    if ( span->end == span->begin + 1 + prefix_len + 1 )
    {
        /* no extra letters */
        /* t.b.d.: get latest version matching prefix */
    }
}

void expand_default( string * input, strspan_t * span )
{
    size_t const prefix_len = macro_type_map[ STR_DEFAULT ].len;

    if ( ! input || ! span || ! span->begin || ! span->end || span->begin >= span->end )
    {
        return;
    }

    if ( span->end > span->begin + 1 + prefix_len + 1 )
    {
        /* variable name not empty */
        /* t.b.d.: Get from g_ctx */
    }
}
