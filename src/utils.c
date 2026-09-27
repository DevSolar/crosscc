#include "utils.h"
#include "support/solog.h"

#include <sys/stat.h>

#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#ifdef _WIN32
#define stat _stat
#endif

int stricmp( char const * lhs, char const * rhs )
{
    SOLOG( TRACE, "stricmp( \"%s\", \"%s\" )", lhs, rhs );

    while ( *lhs && ( tolower( (unsigned char)*lhs ) == tolower( (unsigned char)*rhs ) ) )
    {
        ++lhs;
        ++rhs;
    }

    return (unsigned char)tolower( (unsigned char)*lhs ) - (unsigned char)tolower( (unsigned char)*rhs );
}

int strnicmp( char const * lhs, char const * rhs, size_t n )
{
    SOLOG( TRACE, "strnicmp( \"%s\", \"%s\", %zu )", lhs, rhs, n );

    while ( n && *lhs && ( tolower( (unsigned char)*lhs ) == tolower( (unsigned char)*rhs ) ) )
    {
        ++lhs;
        ++rhs;
        --n;
    }

    if ( n == 0 )
    {
        return 0;
    }
    else
    {
        return (unsigned char)tolower( (unsigned char)*lhs ) - (unsigned char)tolower( (unsigned char)*rhs );
    }
}

int strvicmp( char const * lhs, char const * rhs )
{
    SOLOG( TRACE, "strvicmp( \"%s\", \"%s\" )", lhs, rhs );

    for (;;)
    {
        while ( *lhs && ( tolower( (unsigned char)*lhs ) == tolower( (unsigned char)*rhs ) ) && ! isdigit( (unsigned char)*lhs ) )
        {
            ++lhs;
            ++rhs;
        }

        if ( isdigit( (unsigned char)*lhs ) && isdigit( (unsigned char)*rhs ) )
        {
            long lhv = strtol( lhs, (char **)&lhs, 10 );
            long rhv = strtol( rhs, (char **)&rhs, 10 );

            if ( lhv != rhv )
            {
                return ( lhv > rhv ) ? 1 : -1;
            }
        }
        else
        {
            return (unsigned char)tolower( (unsigned char)*lhs ) - (unsigned char)tolower( (unsigned char)*rhs );
        }
    }
}

char * strrpbrk( strspan_t * span, char const * c )
{
    if ( span->begin && span->end && c && *c && ( span->end > span->begin ) )
    {
        char const * p = span->end - 1;

        while ( p >= span->begin )
        {
            if ( ( *p != '\0' ) && ( strchr( c, *p ) != NULL ) )
            {
                return (char *)p;
            }

            --p;
        }
    }

    return NULL;
}

bool file_readable( char const * filename )
{
    SOLOG( TRACE, "file_readable( \"%s\" )", filename ? filename : "(null)" );

    if ( filename && *filename )
    {
        struct stat st;

        if ( ! stat( filename, &st ) )
        {
            return S_ISREG( st.st_mode ) && ( st.st_mode & S_IRUSR );
        }
    }

    return false;
}

bool dir_readable( char const * dirname )
{
    SOLOG( TRACE, "dir_readable( \"%s\" )", dirname ? dirname : "(null)" );

    if ( dirname && *dirname )
    {
        struct stat st;

        if ( ! stat( dirname, &st ) )
        {
            return S_ISDIR( st.st_mode ) && ( st.st_mode & S_IRUSR );
        }
    }

    return false;
}

void rtrim( char * s )
{
    SOLOG( TRACE, "rtrim( \"%s\" )", s ? s : "(null)" );

    if ( s && *s )
    {
        char * p = s + strlen( s );

        while ( p > s && isspace( (unsigned char)*( p - 1 ) ) )
        {
            --p;
        }

        *p = '\0';
    }
}

void rtrim_str( string * str )
{
    SOLOG( TRACE, "rtrim_str( \"%s\" )", str ? ( size( str ) > 0 ? first( str ) : "" ) : "(null)" );

    if ( str )
    {
        while ( size( str ) > 0 && isspace( (unsigned char)( *last( str ) ) ) )
        {
            erase( str, size( str ) - 1 );
        }
    }
}

char * ltrim( char const * s )
{
    SOLOG( TRACE, "ltrim( \"%s\" )", s ? s : "(null)" );

    if ( s && *s )
    {
        while ( isspace( (unsigned char)*s ) )
        {
            ++s;
        }
    }

    return (char *)s;
}

void ltrim_str( string * str )
{
    SOLOG( TRACE, "ltrim_str( \"%s\" )", str ? ( size( str ) > 0 ? first( str ) : "" ) : "(null)" );

    if ( str && size( str ) > 0 )
    {
        char * s = ltrim( first( str ) );
        size_t n = (size_t)( s - first( str ) );

        if ( n > 0 )
        {
            erase_n( str, 0, n );
        }
    }
}

int get_line( FILE * fh, string * line, int * lineno )
{
    char buf[ 256 ];
    bool continued = false;

    SOLOG( TRACE, "get_line(...)" );

    if ( fh == NULL || line == NULL || lineno == NULL || *lineno == INT_MAX )
    {
        SOLOG( WARN, "Invalid arguments" );
        return -1;
    }

    clear( line );

    for (;;)
    {
        char * p = fgets( buf, sizeof( buf ), fh );

        if ( p == NULL && size( line ) == 0 )
        {
            if ( ferror( fh ) )
            {
                SOLOG( ERR, "Read error: %s", strerror( errno ) );
                return -1;
            }

            SOLOG( TRACE, "end of file" );
            return 0;
        }

        if ( p != NULL )
        {
            if ( continued )
            {
                SOLOG( TRACE, "Read continued line, adding a space where the \"\\\\n\" was" );
                continued = false;
                push_fmt( line, " ", ltrim( buf ) );
            }
            else
            {
                push_fmt( line, buf );
            }

            if ( *last( line ) != '\n' )
            {
                SOLOG( TRACE, "Read long line, continue" );
                continue;
            }
        }

        ++*lineno;
        SOLOG( TRACE, "Completed line #%d", *lineno );
        rtrim_str( line );

        if ( size( line ) && ( *last( line ) == '\\' ) )
        {
            SOLOG( TRACE, "Line backslash-continued" );
            continued = true;
            erase( line, size( line ) - 1 );
            rtrim_str( line );

            if ( p != NULL )
            {
                continue;
            }
        }

        ltrim_str( line );
        return *lineno;
    }
}

int foreach( char const * list, char const * delims, bool (*callback)( string *, void * ), void * userdata )
{
    int count = 0;

    SOLOG( TRACE, "foreach( \"%s\", \"%s\", ... )", list, delims ? delims : "(null)" );

    if ( list && *list && callback )
    {
        strspan_t span = { list, NULL };

        if ( ! delims )
        {
            SOLOG( ERR, "foreach() called with (null) delimiter list" );
            return -1;
        }

        while ( *span.begin )
        {
            /* skip leading delimiters */
            while ( *span.begin && strchr( delims, *span.begin ) != NULL )
            {
                ++span.begin;
            }

            if ( ! *span.begin )
            {
                break;
            }

            if ( *span.begin == '\'' || *span.begin == '"' )
            {
                /* quoted, loop till end quote */
                span.end = span.begin + 1;

                while ( *span.end && *span.end != *span.begin )
                {
                    ++span.end;
                }

                if ( *span.end )
                {
                    string token;
                    init( &token );
                    push_n( &token, span.begin + 1, (size_t)( span.end - span.begin ) - 1 );
                    span.begin = span.end + 1;

                    if ( strchr( delims, *span.begin ) == NULL )
                    {
                        SOLOG( ERR, "Partially quoted token in foreach(), token '%s'", first( &token ) );
                        cleanup( &token );
                        return -1;
                    }
                    else
                    {
                        bool cb_result = callback( &token, userdata );
                        ++count;
                        cleanup( &token );

                        if ( ! cb_result )
                        {
                            SOLOG( TRACE, "callback returned false, breaking loop" );
                            break;
                        }
                    }
                }
                else
                {
                    SOLOG( ERR, "Unmatched quotes in foreach(), token %.*s", (int)( span.end - span.begin ) + 1, span.begin );
                    return -1;
                }
            }
            else
            {
                string token;
                bool cb_result;

                /* unquoted, loop till next delimiter */
                span.end = span.begin + 1;

                while ( *span.end && strchr( delims, *span.end ) == NULL )
                {
                    ++span.end;
                }

                init( &token );
                push_n( &token, span.begin, (size_t)( span.end - span.begin ) );
                span.begin = span.end;
                cb_result = callback( &token, userdata );
                ++count;
                cleanup( &token );

                if ( ! cb_result )
                {
                    SOLOG( TRACE, "callback returned false, breaking loop" );
                    break;
                }
            }
        }
    }

    return count;
}

/* Context structure for foreach callback in find_first */
typedef struct
{
    char const * subpath;
    string * result;
} find_first_cb_t;

/* Callback function invoked by foreach for each directory component in envvar list */
static bool find_first_cb( string * dir_token, void * udata )
{
    find_first_cb_t * ctx = (find_first_cb_t *)udata;
    string path;
    init( &path );

    /* Environment variable */
    if ( dir_token && size( dir_token ) > 0 )
    {
        push_fmt( &path, first( dir_token ) );

        if ( *last( &path ) != '/' && *last( &path ) != '\\' )
        {
            push_fmt( &path, "/" );
        }
    }

    /* Verbatim path */
    if ( ctx->subpath && *ctx->subpath )
    {
        char const * subpath = ctx->subpath;

        if ( ( *subpath == '/' || *subpath == '\\' ) && size( &path ) > 0 )
        {
            ++subpath;
        }

        push_fmt( &path, subpath );
    }

    /* Test */
    if ( file_readable( first( &path ) ) )
    {
        push_fmt( ctx->result, first( &path ) );
    }

    cleanup( &path );
    return size( ctx->result ) == 0;
}

string find_first( vec( candidate_t ) candidates )
{
    string result;
    init( &result );

    /* Iterate through candidate_t list */
    for_each( &candidates, candidate )
    {
        if ( candidate->env && *candidate->env )
        {
            /* Candidate has an environment variable specified */
            char const * envvar = getenv( candidate->env );

            if ( envvar && *envvar )
            {
                /* Environment variable exists and is non-empty:
                 * Use foreach() to split envvar by path delimiter and invoke find_first_cb.
                 */
                find_first_cb_t ctx = { candidate->path, &result };

#ifdef _WIN32
                foreach( envvar, ";", find_first_cb, &ctx );
#else
                foreach( envvar, ":", find_first_cb, &ctx );
#endif

                if ( size( ctx.result ) > 0 )
                {
                    break;
                }
            }
        }
        else if ( candidate->path && *candidate->path )
        {
            /* No environment variable: Check candidate->path directly. */
            if ( file_readable( candidate->path ) )
            {
                push_fmt( &result, candidate->path );
                break;
            }
        }
    }

    return result;
}

/*
string get_setting( char const * section, char const * key )
{
    string result;

    init( &result );

    if ( ( section && *section ) || ( key && *key ) )
    {
        string full_key;
        string * value;

        init( &full_key );

        if ( section && *section )
        {
            push_fmt( &full_key, section );

            if ( key && *key )
            {
                push_fmt( &full_key, "." );
            }
        }

        if ( key && *key )
        {
            push_fmt( &full_key, key );
        }

        value = get( &g_ctx.settings, first( &full_key ) );

        if ( value && size( value ) )
        {
            push_fmt( &result, first( value ) );
        }

        cleanup( &full_key );
    }

    return result;
}
*/
