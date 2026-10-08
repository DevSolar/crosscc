#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "utils.h"
#include "support/solog.h"

#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define stat _stat
#endif

int stricmp( char const * lhs, char const * rhs )
{
    SOLOG( TRACE, "stricmp( \"%s\", \"%s\" )", lhs ? lhs : "(null)", rhs ? rhs : "(null)" );

    if ( ! lhs || ! rhs )
    {
        SOLOG( WARN, "stricmp called with NULL pointer (lhs=%p, rhs=%p)", (void *)lhs, (void *)rhs );
    }

    while ( *lhs && ( tolower( (unsigned char)*lhs ) == tolower( (unsigned char)*rhs ) ) )
    {
        ++lhs;
        ++rhs;
    }

    return (unsigned char)tolower( (unsigned char)*lhs ) - (unsigned char)tolower( (unsigned char)*rhs );
}

int strnicmp( char const * lhs, char const * rhs, size_t n )
{
    SOLOG( TRACE, "strnicmp( \"%s\", \"%s\", %zu )", lhs ? lhs : "(null)", rhs ? rhs : "(null)", n );

    if ( ! lhs || ! rhs )
    {
        SOLOG( WARN, "strnicmp called with NULL pointer (lhs=%p, rhs=%p)", (void *)lhs, (void *)rhs );
    }

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
    SOLOG( TRACE, "strvicmp( \"%s\", \"%s\" )", lhs ? lhs : "(null)", rhs ? rhs : "(null)" );

    if ( ! lhs || ! rhs )
    {
        SOLOG( WARN, "strvicmp called with NULL pointer (lhs=%p, rhs=%p)", (void *)lhs, (void *)rhs );
    }

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
    SOLOG( TRACE, "strrpbrk( %p, \"%s\" )", (void *)span, c ? c : "(null)" );

    if ( ! span || ! c )
    {
        SOLOG( WARN, "strrpbrk called with NULL argument (span=%p, c=%p)", (void *)span, (void *)c );
    }

    if ( span && span->begin && span->end && c && *c && ( span->end > span->begin ) )
    {
        char const * p = span->end - 1;

        while ( p >= span->begin )
        {
            if ( ( *p != '\0' ) && ( strchr( c, *p ) != NULL ) )
            {
                SOLOG( TRACE, "strrpbrk(...) -> %p", (void *)p );
                return (char *)p;
            }

            --p;
        }
    }

    SOLOG( TRACE, "strrpbrk(...) -> NULL" );
    return NULL;
}

long spantol( strspan_t * span, char const ** endp )
{
    char const * p;
    char const * end;
    long rc = 0;
    long const limval = LONG_MAX / 10;
    int const limdigit = (int)( LONG_MAX % 10 );
    int digit = -1;

    SOLOG( TRACE, "spantol( %p, %p )", (void *)span, (void *)endp );

    if ( ! span || ! span->begin || ! span->end )
    {
        SOLOG( WARN, "spantol called with NULL argument (span=%p)", (void *)span );

        if ( endp != NULL )
        {
            *endp = ( span && span->begin ) ? span->begin : NULL;
        }

        return 0;
    }

    if ( span->begin > span->end )
    {
        SOLOG( WARN, "spantol called with invalid span (begin > end)" );

        if ( endp != NULL )
        {
            *endp = span->begin;
        }

        return 0;
    }

    p = span->begin;
    end = span->end;

    while ( ( p < end ) && isdigit( (unsigned char)*p ) )
    {
        digit = *p - '0';

        if ( ( rc < limval ) || ( ( rc == limval ) && ( digit <= limdigit ) ) )
        {
            rc = rc * 10 + digit;
            ++p;
        }
        else
        {
            errno = ERANGE;

            while ( ( p < end ) && isdigit( (unsigned char)*p ) )
            {
                ++p;
            }

            rc = LONG_MAX;
            break;
        }
    }

    if ( digit == -1 )
    {
        if ( endp != NULL )
        {
            *endp = span->begin;
        }

        return 0;
    }

    if ( endp != NULL )
    {
        *endp = p;
    }

    SOLOG( TRACE, "spantol(...) -> %ld", rc );
    return rc;
}

bool file_readable( char const * filename )
{
    SOLOG( TRACE, "file_readable( \"%s\" )", filename ? filename : "(null)" );

    if ( filename && *filename )
    {
        struct stat st;

        if ( ! stat( filename, &st ) )
        {
            bool readable = S_ISREG( st.st_mode ) && ( st.st_mode & S_IRUSR );
            SOLOG( TRACE, "file_readable( \"%s\" ) -> %s", filename, readable ? "true" : "false" );
            return readable;
        }

        SOLOG( TRACE, "stat( \"%s\" ) failed: %s", filename, strerror( errno ) );
    }

    SOLOG( TRACE, "file_readable( \"%s\" ) -> false", filename ? filename : "(null)" );
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
            bool readable = S_ISDIR( st.st_mode ) && ( st.st_mode & S_IRUSR );
            SOLOG( TRACE, "dir_readable( \"%s\" ) -> %s", dirname, readable ? "true" : "false" );
            return readable;
        }

        SOLOG( TRACE, "stat( \"%s\" ) failed: %s", dirname, strerror( errno ) );
    }

    SOLOG( TRACE, "dir_readable( \"%s\" ) -> false", dirname ? dirname : "(null)" );
    return false;
}

/* Return true if file 'lhs' has a modification date
 * newer than that of file 'rhs'
 */
bool is_newer( char const * lhs, char const * rhs )
{
    struct stat st_lhs;
    struct stat st_rhs;
    bool newer;

    SOLOG( TRACE, "is_newer( \"%s\", \"%s\" )", lhs ? lhs : "(null)", rhs ? rhs : "(null)" );

    if ( ! lhs || ! *lhs || ! rhs || ! *rhs )
    {
        SOLOG( TRACE, "is_newer(...) -> false (invalid argument)" );
        return false;
    }

    if ( stat( lhs, &st_lhs ) != 0 )
    {
        SOLOG( TRACE, "stat( \"%s\" ) failed: %s", lhs, strerror( errno ) );
        return false;
    }

    if ( stat( rhs, &st_rhs ) != 0 )
    {
        if ( errno == ENOENT )
        {
            SOLOG( TRACE, "rhs \"%s\" does not exist; lhs is newer -> true", rhs );
            return true;
        }

        SOLOG( TRACE, "stat( \"%s\" ) failed: %s", rhs, strerror( errno ) );
        return false;
    }

#ifdef _WIN32
    newer = st_lhs.st_mtime > st_rhs.st_mtime;
#else
    if ( st_lhs.st_mtime != st_rhs.st_mtime )
    {
        newer = st_lhs.st_mtime > st_rhs.st_mtime;
    }
    else
    {
        newer = st_lhs.st_mtim.tv_nsec > st_rhs.st_mtim.tv_nsec;
    }
#endif

    SOLOG( TRACE, "is_newer( \"%s\", \"%s\" ) -> %s", lhs, rhs, newer ? "true" : "false" );
    return newer;
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

    if ( fh == NULL || line == NULL || lineno == NULL )
    {
        SOLOG( WARN, "get_line() called with invalid NULL argument (fh=%p, line=%p, lineno=%p)",
               (void *)fh, (void *)line, (void *)lineno );
        return -1;
    }

    if ( *lineno == INT_MAX )
    {
        SOLOG( WARN, "get_line() lineno counter reached INT_MAX" );
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

    SOLOG( TRACE, "foreach( \"%s\", \"%s\", ... )", list ? list : "(null)", delims ? delims : "(null)" );

    if ( ! callback )
    {
        SOLOG( ERR, "foreach() called with (null) callback" );
        return -1;
    }

    if ( list && *list )
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
                        bool cb_result;
                        SOLOG( TRACE, "foreach parsed quoted token '%s'", first( &token ) );
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
                SOLOG( TRACE, "foreach parsed token '%s'", first( &token ) );
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
    else
    {
        SOLOG( TRACE, "foreach() list is empty or NULL" );
    }

    SOLOG( TRACE, "foreach() -> %d tokens processed", count );
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

    SOLOG( TRACE, "find_first_cb testing path: \"%s\"", first( &path ) );

    /* Test */
    if ( file_readable( first( &path ) ) )
    {
        SOLOG( DEBUG, "find_first_cb matched readable file: \"%s\"", first( &path ) );
        push_fmt( ctx->result, first( &path ) );
    }
    else
    {
        SOLOG( TRACE, "find_first_cb path not readable: \"%s\"", first( &path ) );
    }

    cleanup( &path );
    return size( ctx->result ) == 0;
}

string find_first( vec( candidate_t ) candidates )
{
    string result;
    init( &result );

    SOLOG( TRACE, "find_first( %zu candidates )", size( &candidates ) );

    /* Iterate through candidate_t list */
    for_each( &candidates, candidate )
    {
        if ( candidate->env && *candidate->env )
        {
            /* Candidate has an environment variable specified */
            char const * envvar = getenv( candidate->env );

            SOLOG( DEBUG, "find_first checking envvar '%s' (value: '%s') with subpath '%s'",
                   candidate->env, envvar ? envvar : "(null)", candidate->path ? candidate->path : "" );

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
                    SOLOG( DEBUG, "find_first candidate '%s' resolved to: \"%s\"", candidate->env, first( &result ) );
                    break;
                }
            }
            else
            {
                SOLOG( DEBUG, "find_first envvar '%s' is not set or empty, skipping candidate", candidate->env );
            }
        }
        else if ( candidate->path && *candidate->path )
        {
            SOLOG( DEBUG, "find_first checking verbatim path '%s'", candidate->path );

            /* No environment variable: Check candidate->path directly. */
            if ( file_readable( candidate->path ) )
            {
                SOLOG( DEBUG, "find_first verbatim path matched: \"%s\"", candidate->path );
                push_fmt( &result, candidate->path );
                break;
            }
        }
        else
        {
            SOLOG( WARN, "find_first encountered candidate with neither envvar nor path" );
        }
    }

    if ( size( &result ) == 0 )
    {
        SOLOG( DEBUG, "find_first found no matching candidate" );
    }

    SOLOG( TRACE, "find_first(...) -> \"%s\"", first( &result ) );
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
