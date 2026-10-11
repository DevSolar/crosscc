#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <sys/utime.h>
#else
#define sleep system_sleep
#define link system_link
#include <unistd.h>
#undef sleep
#undef link

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <spawn.h>
#include <time.h>
#include <utime.h>
#endif

#include "tools.h"
#include "execute.h"
#include "support/solog.h"
#include "utils.h"

#define LONESHA256_STATIC
#define LONESHA256_STDIO

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

#include "support/lonesha256.h"

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

static bool copy_file( char const * src, char const * dst )
{
#ifdef _WIN32
    if ( ! CopyFileA( src, dst, FALSE ) )
    {
        SOLOG( ERR, "copy: failed to copy '%s' to '%s': error %lu", src, dst, GetLastError() );
        return false;
    }

    return true;
#else
    FILE * in;
    FILE * out;
    char buf[65536];
    size_t n;
    struct stat st;

    in = fopen( src, "rb" );

    if ( ! in )
    {
        SOLOG( ERR, "copy: failed to open source '%s': %s", src, strerror( errno ) );
        return false;
    }

    out = fopen( dst, "wb" );

    if ( ! out )
    {
        SOLOG( ERR, "copy: failed to open destination '%s': %s", dst, strerror( errno ) );
        fclose( in );
        return false;
    }

    while ( ( n = fread( buf, 1, sizeof( buf ), in ) ) > 0 )
    {
        if ( fwrite( buf, 1, n, out ) != n )
        {
            SOLOG( ERR, "copy: write error to '%s': %s", dst, strerror( errno ) );
            fclose( in );
            fclose( out );
            return false;
        }
    }

    fclose( in );
    fclose( out );

    if ( stat( src, &st ) == 0 )
    {
        chmod( dst, st.st_mode );
    }

    return true;
#endif
}

#ifdef _WIN32
static bool copy_dir_recursive( char const * src, char const * dst )
{
    char search_path[MAX_PATH];
    WIN32_FIND_DATAA fd;
    HANDLE hFind;
    bool success = true;

    CreateDirectoryA( dst, NULL );

    snprintf( search_path, sizeof( search_path ), "%s\\*", src );
    hFind = FindFirstFileA( search_path, &fd );

    if ( hFind == INVALID_HANDLE_VALUE )
    {
        return false;
    }

    do
    {
        char src_sub[MAX_PATH];
        char dst_sub[MAX_PATH];

        if ( strcmp( fd.cFileName, "." ) == 0 || strcmp( fd.cFileName, ".." ) == 0 )
        {
            continue;
        }

        snprintf( src_sub, sizeof( src_sub ), "%s\\%s", src, fd.cFileName );
        snprintf( dst_sub, sizeof( dst_sub ), "%s\\%s", dst, fd.cFileName );

        if ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {
            if ( ! copy_dir_recursive( src_sub, dst_sub ) )
            {
                success = false;
            }
        }
        else
        {
            if ( ! copy_file( src_sub, dst_sub ) )
            {
                success = false;
            }
        }
    } while ( FindNextFileA( hFind, &fd ) );

    FindClose( hFind );
    return success;
}
#else
static bool copy_dir_recursive( char const * src, char const * dst )
{
    DIR * dir;
    struct dirent * entry;
    struct stat st;
    bool success = true;

    dir = opendir( src );

    if ( ! dir )
    {
        return false;
    }

    if ( stat( src, &st ) == 0 )
    {
        mkdir( dst, st.st_mode );
    }
    else
    {
        mkdir( dst, 0777 );
    }

    while ( ( entry = readdir( dir ) ) != NULL )
    {
        char src_sub[PATH_MAX];
        char dst_sub[PATH_MAX];

        if ( strcmp( entry->d_name, "." ) == 0 || strcmp( entry->d_name, ".." ) == 0 )
        {
            continue;
        }

        snprintf( src_sub, sizeof( src_sub ), "%s/%s", src, entry->d_name );
        snprintf( dst_sub, sizeof( dst_sub ), "%s/%s", dst, entry->d_name );

        if ( lstat( src_sub, &st ) != 0 )
        {
            success = false;
            continue;
        }

        if ( S_ISDIR( st.st_mode ) )
        {
            if ( ! copy_dir_recursive( src_sub, dst_sub ) )
            {
                success = false;
            }
        }
        else
        {
            if ( ! copy_file( src_sub, dst_sub ) )
            {
                success = false;
            }
        }
    }

    closedir( dir );
    return success;
}
#endif

static bool make_dir_recursive( char const * path )
{
    char tmp[4096];
    size_t len;
    char * p;

    if ( ! path || ! *path )
    {
        return false;
    }

    len = strlen( path );

    if ( len >= sizeof( tmp ) )
    {
        return false;
    }

    memcpy( tmp, path, len + 1 );

    p = tmp;

#ifdef _WIN32
    if ( ( ( p[0] >= 'a' && p[0] <= 'z' ) || ( p[0] >= 'A' && p[0] <= 'Z' ) ) && p[1] == ':' )
    {
        p += 2;
    }
#endif
    while ( *p == '/' || *p == '\\' )
    {
        ++p;
    }

    for ( ; *p; ++p )
    {
        if ( *p == '/' || *p == '\\' )
        {
            char slash = *p;
            *p = '\0';
#ifdef _WIN32
            CreateDirectoryA( tmp, NULL );
#else
            mkdir( tmp, 0777 );
#endif
            *p = slash;
        }
    }

#ifdef _WIN32
    if ( ! CreateDirectoryA( tmp, NULL ) )
    {
        DWORD err = GetLastError();

        if ( err != ERROR_ALREADY_EXISTS )
        {
            return false;
        }
    }
#else
    if ( mkdir( tmp, 0777 ) != 0 )
    {
        if ( errno != EEXIST )
        {
            return false;
        }
    }
#endif

    return true;
}

#ifdef _WIN32
static bool remove_all_recursive( char const * path, bool force )
{
    DWORD attr = GetFileAttributesA( path );

    if ( attr == INVALID_FILE_ATTRIBUTES )
    {
        return force;
    }

    if ( attr & FILE_ATTRIBUTE_DIRECTORY )
    {
        char search_path[MAX_PATH];
        WIN32_FIND_DATAA fd;
        HANDLE hFind;
        bool ok = true;

        snprintf( search_path, sizeof( search_path ), "%s\\*", path );
        hFind = FindFirstFileA( search_path, &fd );

        if ( hFind != INVALID_HANDLE_VALUE )
        {
            do
            {
                char child[MAX_PATH];

                if ( strcmp( fd.cFileName, "." ) == 0 || strcmp( fd.cFileName, ".." ) == 0 )
                {
                    continue;
                }

                snprintf( child, sizeof( child ), "%s\\%s", path, fd.cFileName );

                if ( ! remove_all_recursive( child, force ) )
                {
                    ok = false;
                }
            } while ( FindNextFileA( hFind, &fd ) );

            FindClose( hFind );
        }
        if ( ! RemoveDirectoryA( path ) && ! force )
        {
            ok = false;
        }

        return ok;
    }
    else
    {
        if ( ! DeleteFileA( path ) && ! force )
        {
            return false;
        }

        return true;
    }
}
#else
static bool remove_all_recursive( char const * path, bool force )
{
    struct stat st;
    DIR * dir;
    struct dirent * entry;
    bool ok = true;

    if ( lstat( path, &st ) != 0 )
    {
        return force;
    }

    if ( S_ISDIR( st.st_mode ) )
    {
        dir = opendir( path );

        if ( ! dir )
        {
            return force;
        }

        while ( ( entry = readdir( dir ) ) != NULL )
        {
            char child[PATH_MAX];

            if ( strcmp( entry->d_name, "." ) == 0 || strcmp( entry->d_name, ".." ) == 0 )
            {
                continue;
            }

            snprintf( child, sizeof( child ), "%s/%s", path, entry->d_name );

            if ( ! remove_all_recursive( child, force ) )
            {
                ok = false;
            }
        }
        closedir( dir );

        if ( rmdir( path ) != 0 && ! force )
        {
            ok = false;
        }

        return ok;
    }
    else
    {
        if ( unlink( path ) != 0 && ! force )
        {
            return false;
        }

        return true;
    }
}
#endif

bool concat( vec( string ) * options )
{
    bool rc = true;
    char buf[65536];
    size_t i;
    size_t count;
    string * opt;
    char const * filename;
    FILE * f;
    size_t n;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "concat" );
        return false;
    }

    count = size( options );
    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        filename = first( opt );
        f = fopen( filename, "rb" );

        if ( ! f )
        {
            SOLOG( ERR, "concat: failed to open '%s': %s", filename, strerror( errno ) );
            rc = false;
            continue;
        }

        while ( ( n = fread( buf, 1, sizeof( buf ), f ) ) > 0 )
        {
            if ( fwrite( buf, 1, n, stdout ) != n )
            {
                SOLOG( ERR, "concat: write error on '%s': %s", filename, strerror( errno ) );
                rc = false;
                break;
            }
        }

        fclose( f );
    }

    return rc;
}

bool copy( vec( string ) * options )
{
    string * src;
    string * dst;

    tool_flag_t flags[] =
    {
        { "--if-newer", false },
        { NULL, false }
    };

    extract_flags( options, flags );

    if ( size( options ) != 2 )
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "copy", 2, size( options ) );
        return false;
    }

    src = get( options, 0 );
    dst = get( options, 1 );

    if ( flags[0].is_set && ! is_newer( first( src ), first( dst ) ) )
    {
        return true;
    }

    return copy_file( first( src ), first( dst ) );
}

bool copydir( vec( string ) * options )
{
    string * src;
    string * dst;

    if ( size( options ) != 2 )
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "copydir", 2, size( options ) );
        return false;
    }

    src = get( options, 0 );
    dst = get( options, 1 );

    return copy_dir_recursive( first( src ), first( dst ) );
}

bool echo( vec( string ) * options )
{
    size_t count = size( options );
    size_t i;
    string * opt;
    char const * s;

    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        s = first( opt );

        if ( s )
        {
            fputs( s, stdout );
        }

        if ( i + 1 < count )
        {
            putchar( ' ' );
        }
    }

    putchar( '\n' );
    fflush( stdout );
    return true;
}

bool unset_env( vec( string ) * options )
{
    bool rc = true;
    size_t i;
    size_t count;
    string * opt;
    char const * name;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "unset_env" );
        return false;
    }

    count = size( options );

    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        name = first( opt );

        if ( name && *name )
        {
#ifdef _WIN32
            if ( ! SetEnvironmentVariableA( name, NULL ) )
            {
                SOLOG( ERR, "unset_env: failed to unset '%s'", name );
                rc = false;
            }
#else
            if ( unsetenv( name ) != 0 )
            {
                SOLOG( ERR, "unset_env: failed to unset '%s': %s", name, strerror( errno ) );
                rc = false;
            }
#endif
        }
    }

    return rc;
}

bool set_env( vec( string ) * options )
{
    string * opt0;
    string * opt1;
    char const * key;
    char const * val;

    if ( size( options ) != 2 )
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "set_env", 2, size( options ) );
        return false;
    }

    opt0 = get( options, 0 );
    opt1 = get( options, 1 );
    key = first( opt0 );
    val = first( opt1 );

#ifdef _WIN32
    if ( ! SetEnvironmentVariableA( key, val ) )
    {
        SOLOG( ERR, "set_env: failed to set '%s'='%s'", key, val );
        return false;
    }
#else
    if ( setenv( key, val, 1 ) != 0 )
    {
        SOLOG( ERR, "set_env: failed to set '%s'='%s': %s", key, val, strerror( errno ) );
        return false;
    }
#endif

    return true;
}

bool makedir( vec( string ) * options )
{
    bool rc = true;
    size_t i;
    size_t count;
    string * opt;
    char const * dir;

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "makedir" );
        return false;
    }

    count = size( options );

    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        dir = first( opt );
        if ( ! make_dir_recursive( dir ) )
        {
            SOLOG( ERR, "makedir: failed to create directory '%s'", dir );
            rc = false;
        }
    }

    return rc;
}

bool checksum( vec( string ) * options )
{
    string * opt;
    char const * filename;
    unsigned char hash[32];
    int rc;
    int i;

    if ( size( options ) != 1 )
    {
        SOLOG( ERR, "'%s' expects 1 argument; %zu provided", "checksum", size( options ) );
        return false;
    }

    opt = get( options, 0 );
    filename = first( opt );

    rc = lonesha256_file( hash, filename );

    if ( rc != 0 )
    {
        SOLOG( ERR, "checksum: failed to open '%s': %s", filename, strerror( errno ) );
        return false;
    }

    for ( i = 0; i < 32; ++i )
    {
        printf( "%02x", hash[i] );
    }

    printf( "  %s\n", filename );
    return true;
}

bool rm( vec( string ) * options )
{
    bool rc = true;
    bool force;
    bool all;
    size_t i;
    size_t count;
    string * opt;
    char const * path;
#ifdef _WIN32
    DWORD attr;
#else
    struct stat st;
#endif

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
        return false;
    }

    force = flags[0].is_set;
    all = flags[1].is_set;
    count = size( options );

    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        path = first( opt );
#ifdef _WIN32
        attr = GetFileAttributesA( path );
        if ( attr == INVALID_FILE_ATTRIBUTES )
        {
            if ( ! force )
            {
                SOLOG( ERR, "rm: cannot remove '%s': file not found", path );
                rc = false;
            }

            continue;
        }

        if ( attr & FILE_ATTRIBUTE_DIRECTORY )
        {
            if ( ! all )
            {
                SOLOG( ERR, "rm: cannot remove directory '%s' without --all", path );
                rc = false;
                continue;
            }

            if ( ! remove_all_recursive( path, force ) )
            {
                rc = false;
            }
        }
        else
        {
            if ( ! DeleteFileA( path ) && ! force )
            {
                SOLOG( ERR, "rm: failed to delete '%s'", path );
                rc = false;
            }
        }
#else
        if ( lstat( path, &st ) != 0 )
        {
            if ( ! force )
            {
                SOLOG( ERR, "rm: cannot remove '%s': %s", path, strerror( errno ) );
                rc = false;
            }

            continue;
        }

        if ( S_ISDIR( st.st_mode ) )
        {
            if ( ! all )
            {
                SOLOG( ERR, "rm: cannot remove directory '%s' without --all", path );
                rc = false;
                continue;
            }

            if ( ! remove_all_recursive( path, force ) )
            {
                rc = false;
            }
        }
        else
        {
            if ( unlink( path ) != 0 && ! force )
            {
                SOLOG( ERR, "rm: failed to unlink '%s': %s", path, strerror( errno ) );
                rc = false;
            }
        }
#endif
    }

    return rc;
}

bool sleep( vec( string ) * options )
{
    char * endp = NULL;
    string * opt;
    char const * str_val;
    double secs;

    if ( size( options ) != 1 )
    {
        SOLOG( ERR, "'%s' expects 1 argument; %zu provided", "sleep", size( options ) );
        return false;
    }

    opt = get( options, 0 );
    str_val = first( opt );
    secs = strtod( str_val, &endp );

    if ( endp == str_val || secs < 0.0 )
    {
        SOLOG( ERR, "sleep: invalid duration '%s'", str_val );
        return false;
    }

#ifdef _WIN32
    Sleep( (DWORD)( secs * 1000.0 ) );
#else
    {
        struct timespec req;
        req.tv_sec = (time_t)secs;
        req.tv_nsec = (long)( ( secs - (double)req.tv_sec ) * 1e9 );

        while ( nanosleep( &req, &req ) != 0 && errno == EINTR )
        {
            /* Retry on EINTR */
        }
    }
#endif
    return true;
}

bool touch( vec( string ) * options )
{
    bool rc = true;
    bool create;
    size_t i;
    size_t count;
    string * opt;
    char const * path;
#ifdef _WIN32
    DWORD attr;
#else
    struct stat st;
#endif

    tool_flag_t flags[] =
    {
        { "--create", false },
        { NULL, false }
    };

    extract_flags( options, flags );

    if ( size( options ) == 0 )
    {
        SOLOG( ERR, "'%s' expects at least 1 argument; 0 provided", "touch" );
        return false;
    }

    create = flags[0].is_set;
    count = size( options );

    for ( i = 0; i < count; ++i )
    {
        opt = get( options, i );
        path = first( opt );

        if ( ! path || ! *path )
        {
            continue;
        }

#ifdef _WIN32
        attr = GetFileAttributesA( path );
        if ( attr == INVALID_FILE_ATTRIBUTES )
        {
            if ( create )
            {
                FILE * f = fopen( path, "ab" );

                if ( f )
                {
                    fclose( f );
                }
                else
                {
                    SOLOG( ERR, "touch: failed to create '%s'", path );
                    rc = false;
                    continue;
                }
            }
            else
            {
                SOLOG( ERR, "touch: file '%s' does not exist", path );
                rc = false;
                continue;
            }
        }

        if ( _utime( path, NULL ) != 0 )
        {
            SOLOG( ERR, "touch: failed to update timestamp on '%s'", path );
            rc = false;
        }
#else
        if ( stat( path, &st ) != 0 )
        {
            if ( create )
            {
                FILE * f = fopen( path, "ab" );

                if ( f )
                {
                    fclose( f );
                }
                else
                {
                    SOLOG( ERR, "touch: failed to create '%s': %s", path, strerror( errno ) );
                    rc = false;
                    continue;
                }
            }
            else
            {
                SOLOG( ERR, "touch: file '%s' does not exist", path );
                rc = false;
                continue;
            }
        }

        if ( utime( path, NULL ) != 0 )
        {
            SOLOG( ERR, "touch: failed to update timestamp on '%s': %s", path, strerror( errno ) );
            rc = false;
        }
#endif
    }

    return rc;
}

bool link( vec( string ) * options )
{
    string * opt0;
    string * opt1;
    char const * link_name;
    char const * target;

    if ( size( options ) != 2 )
    {
        SOLOG( ERR, "'%s' expects %d arguments; %zu provided", "link", 2, size( options ) );
        return false;
    }

    opt0 = get( options, 0 );
    opt1 = get( options, 1 );
    link_name = first( opt0 );
    target = first( opt1 );

#ifdef _WIN32
    {
        DWORD flags = 0;
        DWORD attr = GetFileAttributesA( target );

        if ( attr != INVALID_FILE_ATTRIBUTES && ( attr & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            flags |= 1; /* SYMBOLIC_LINK_FLAG_DIRECTORY */
        }

        if ( ! CreateSymbolicLinkA( link_name, target, flags ) )
        {
            if ( ! CreateHardLinkA( link_name, target, NULL ) )
            {
                return copy_file( target, link_name );
            }
        }
    }
#else
    if ( symlink( target, link_name ) != 0 )
    {
        if ( linkat( AT_FDCWD, target, AT_FDCWD, link_name, 0 ) != 0 )
        {
            return copy_file( target, link_name );
        }
    }
#endif
    return true;
}

bool unpack( vec( string ) * options )
{
    vec( string ) cmdline;
    int code;
    string * opt0;

    if ( size( options ) < 1 || size( options ) > 2 )
    {
        SOLOG( ERR, "'%s' expects 1 or 2 arguments; %zu provided", "unpack", size( options ) );
        return false;
    }

    opt0 = get( options, 0 );

    init( &cmdline );
    push( &cmdline, "tar" );
    push( &cmdline, "-xf" );
    push( &cmdline, first( opt0 ) );

    if ( size( options ) == 2 )
    {
        string * opt1 = get( options, 1 );
        push( &cmdline, "-C" );
        push( &cmdline, first( opt1 ) );
    }

    code = execute( &cmdline );
    cleanup( &cmdline );
    return code == 0;
}
