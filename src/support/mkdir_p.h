/*
 * mkdir_p.h - header-only C library: create a directory and all missing
 *             parents (like `mkdir -p`), on POSIX and Windows.
 *
 * USAGE
 *   In exactly ONE C or C++ source file, define MKDIR_P_IMPLEMENTATION before
 *   including this header to create the implementation:
 *
 *       #define MKDIR_P_IMPLEMENTATION
 *       #include "mkdir_p.h"
 *
 *   In all other files, just include the header normally.
 *
 *       if (mkdir_p("a/b/c", 0777) != 0)
 *       {
 *           perror("mkdir_p");
 *       }
 *
 * API
 *   int mkdir_p(const char *path, unsigned int mode);
 *
 *     Creates `path` and any missing parent directories.
 *     Returns 0 on success (including if the directory already exists),
 *     or -1 with errno set on failure:
 *       EINVAL   path is NULL or empty
 *       ENOMEM   out of memory
 *       ENOTDIR  a component of the path exists but is not a directory
 *       others   as set by mkdir()/_mkdir() (EACCES, ENOENT, ...)
 *     `mode` is passed to mkdir() on POSIX (subject to umask) and ignored on
 *     Windows. Both '/' and '\' are accepted as separators on Windows; drive
 *     ("C:") and UNC ("\\server\share") prefixes are not created.
 *
 * OPTIONS (define before including)
 *   MKDIR_P_IMPLEMENTATION  Emit the function bodies in this translation unit.
 *   MKDIR_P_STATIC          Make all functions `static` (implementation is
 *                           then private to the including file).
 *   MKDIR_P_API             Override the linkage/visibility decoration.
 *
 * LICENSE
 *   CC0 1.0 Universal (Public Domain Dedication).
 *   To the extent possible under law, the author(s) have waived all copyright
 *   and related or neighboring rights to this work. See the file LICENSE.CC0 or
 *   <https://creativecommons.org/publicdomain/zero/1.0/>.
 *
 *   SPDX-License-Identifier: CC0-1.0
 *
 * VERSION
 *   1.0
 */

#ifndef MKDIR_P_H
#define MKDIR_P_H

#define MKDIR_P_VERSION_MAJOR 1
#define MKDIR_P_VERSION_MINOR 0

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MKDIR_P_API
#  ifdef MKDIR_P_STATIC
#    define MKDIR_P_API static
#  else
#    define MKDIR_P_API extern
#  endif
#endif

/* ---------------------------------------------------------------------------
 * Public API declarations
 * ------------------------------------------------------------------------ */

MKDIR_P_API int mkdir_p(const char *path, unsigned int mode);

#ifdef __cplusplus
}
#endif

#endif /* MKDIR_P_H */

/* ---------------------------------------------------------------------------
 * Implementation
 * ------------------------------------------------------------------------ */

#ifdef MKDIR_P_IMPLEMENTATION

#ifndef MKDIR_P_IMPLEMENTATION_DEFINED
#define MKDIR_P_IMPLEMENTATION_DEFINED

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#  include <direct.h>
#  define MKDIR_P__MKDIR(p, m) _mkdir(p)
#  define MKDIR_P__STAT        _stat
#  define MKDIR_P__STAT_T      struct _stat
#  define MKDIR_P__ISDIR(m)    (((m) & _S_IFMT) == _S_IFDIR)
#else
#  define MKDIR_P__MKDIR(p, m) mkdir((p), (mode_t)(m))
#  define MKDIR_P__STAT        stat
#  define MKDIR_P__STAT_T      struct stat
#  define MKDIR_P__ISDIR(m)    S_ISDIR(m)
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int mkdir_p__is_sep( char c )
{
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

static int mkdir_p__is_dir( char const * path )
{
    MKDIR_P__STAT_T st;
    return MKDIR_P__STAT( path, &st ) == 0 && MKDIR_P__ISDIR( st.st_mode );
}

/* Create a single directory; an already existing directory is success. */
static int mkdir_p__make_one( char const * path, unsigned int mode )
{
    if ( MKDIR_P__MKDIR( path, mode ) == 0 )
    {
        return 0;
    }

    if ( errno != EEXIST )
    {
        return -1;
    }

    if ( mkdir_p__is_dir( path ) )
    {
        return 0;
    }

    errno = ENOTDIR;
    return -1;
}

/* Returns the first character after the non-creatable root prefix. */
static char * mkdir_p__skip_prefix( char * p )
{
#ifdef _WIN32
    if ( mkdir_p__is_sep( p[ 0 ] ) && mkdir_p__is_sep( p[ 1 ] ) && p[ 2 ] && ! mkdir_p__is_sep( p[ 2 ] ) )
    {
        /* UNC: \\server\share\ */
        int i;
        p += 2;
        for ( i = 0; i < 2; i++ )
        {
            while ( *p && ! mkdir_p__is_sep( *p ) )
            {
                p++;
            }

            if ( i == 0 )
            {
                while ( mkdir_p__is_sep( *p ) )
                {
                    p++;
                }
            }
        }
    }
    else if ( ( ( p[ 0 ] >= 'A' && p[ 0 ] <= 'Z' ) || ( p[ 0 ] >= 'a' && p[ 0 ] <= 'z' ) ) && p[ 1 ] == ':' )
    {
        p += 2; /* drive letter */
    }
#endif

    while ( mkdir_p__is_sep( *p ) )
    {
        p++;
    }

    return p;
}

MKDIR_P_API int mkdir_p( char const * path, unsigned int mode )
{
    char *buf, *start, *p;
    int rc = 0, saved_errno = 0;

    if ( path == NULL || *path == '\0' )
    {
        errno = EINVAL;
        return -1;
    }

    buf = (char *)malloc( strlen( path ) + 1 );

    if ( buf == NULL )
    {
        errno = ENOMEM;
        return -1;
    }

    strcpy( buf, path );

    start = mkdir_p__skip_prefix( buf );

    for ( p = start;; p++ )
    {
        char c = *p;

        if ( c == '\0' || mkdir_p__is_sep( c ) )
        {
            /* Skip empty components (consecutive separators). */
            if ( p > start && ! mkdir_p__is_sep( p[ -1 ] ) )
            {
                *p = '\0';

                if ( mkdir_p__make_one( buf, mode ) != 0 )
                {
                    rc = -1;
                    saved_errno = errno;
                    break;
                }

                *p = c;
            }

            if ( c == '\0' )
            {
                break;
            }
        }
    }

    free( buf );

    if ( rc != 0 )
    {
        errno = saved_errno;
    }

    return rc;
}

#ifdef __cplusplus
}
#endif

#undef MKDIR_P__MKDIR
#undef MKDIR_P__STAT
#undef MKDIR_P__STAT_T
#undef MKDIR_P__ISDIR

#endif /* MKDIR_P_IMPLEMENTATION_DEFINED */

#endif /* MKDIR_P_IMPLEMENTATION */
