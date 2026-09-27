/* SoLog
 *
 * Some Ordinary Logging
 *
 * CC0 1.0 Universal / Public Domain
 *
 * USAGE:
 *
 * * All translation units except for one just include this header and use
 *   SOLOG( <level>, <fmt_string>, ... ), printf-style.
 *   Solog prefixes the user-provided format string with the log line
 *   header; by default via alloca() and truncating too-long format strings,
 *   optionally by falling back to malloc().
 *
 * * Define SOLOG_IMPLEMENTATION in exactly one translation unit before
 *   including this header.
 *
 * * Basic functionality: SOLOG( <lvl>, <fmt>, ... )
 *   * <lvl> can be one of TRACE, DEBUG, INFO, WARN, ERR, FAIL
 *   * printf-style format string and argument list
 *   * single (and thus atomic) call to vfprintf()
 *   * current logging level can be set by solog_config.level = SOLOG_LVL_*
 *     (default: SOLOG_LVL_INFO)
 *   * output stream can be set by solog_config.stream = ...
 *     (default: stderr)
 *   * can override macro solog_alloca to define custom memory allocation
 *
 * * SOLOG_H can be used for version-checking; will be incremented in future
 *   versions.
 *
 * * By setting SOLOG_IMPLEMENTATION to an OR-ed selection of features
 *   (SOLOG_FEATURE_*), you activate those features at compile time; by
 *   AND-ing them out of solog_config.features you can deactivate them
 *   at runtime.
 *
 *   These features are available:
 *
 *   * SOLOG_FEATURE_LEVEL: Show the level of the message in the output.
 *   * SOLOG_FEATURE_MALLOC: Use malloc() for long messages (default is
 *     to truncate the format string after SOLOG_ALLOCA_MAX characters).
 *     Can override macros solog_malloc and solog_free to define custom
 *     memory allocation.
 *     Can override SOLOG_ALLOCA_MAX to set the maximum amount of memory
 *     retrieved from stack before switching to malloc (default 1kB).
 *   * SOLOG_FEATURE_COLOR: Using ANSI escape sequences to colorize logs.
 *   * SOLOG_FEATURE_FILE: Add __FILE__ to the log output.
 *   * SOLOG_FEATURE_FUNC: Add __func__ to the log output.
 *   * SOLOG_FEATURE_LINE: Add __LINE__ to the log output.
 *   * SOLOG_FEATURE_TIME: Add timestamp to the log output.
 *   * SOLOG_FEATURE_DATE: Add datestamp to the log output (automatically
 *     enables timestamp)
 *   * SOLOG_FEATURE_ALL: Enable all the features above.
 *
 * HISTORY:
 *
 *   10 -- Dropped Windows-specific _malloca() usage in favor of unified
 *         implementation of dynamic memory allocation
 *    9 -- Fixed issues with Windows compilation, added greatest.h
 *    8 -- Fixed issue #1, DATE / TIME logic inversed
 *    7 -- Bugfixes
 *    6 -- Optional time/datestamping
 *    5 -- Optional file name, function name, line number of SOLOG() call
 *    4 -- Optional colorized output
 *    3 -- Optional use of malloc() for very long messages
 *    2 -- Making level output optional, preparing for extension
 *    1 -- Initial release, basic functionality
 *
 * FUTURE:
 *
 * * scoping (module-local log levels)
 * * ...
 *
 * Latest version available at:
 *   https://github.com/DevSolar/solog/
 */

/* ---------------------------------------------------------------------- */

#ifndef SOLOG_H
#define SOLOG_H 10

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/* LOG LEVELS */
/* SOLOG_LVL_* automatically prefixed by SOLOG() macro */
typedef enum
{
    SOLOG_LVL_TRACE = 0,
    SOLOG_LVL_DEBUG,
    SOLOG_LVL_INFO,
    SOLOG_LVL_WARN,
    SOLOG_LVL_ERR,
    SOLOG_LVL_FAIL
} solog_level_t;


/* COMPILE TIME CONFIGURATION */
/* Display the message level in the output */
#define SOLOG_FEATURE_LEVEL (1<<0)
#define SOLOG_FEATURE_MALLOC (1<<1)
#define SOLOG_FEATURE_COLOR (1<<2)
#define SOLOG_FEATURE_FILE (1<<3)
#define SOLOG_FEATURE_FUNC (1<<4)
#define SOLOG_FEATURE_LINE (1<<5)
#define SOLOG_FEATURE_TIME (1<<6)
#define SOLOG_FEATURE_DATE (1<<7)

#define SOLOG_FEATURE_ALL ((1<<8)-1)

/* RUNTIME CONFIGURATION */
/* Struct gets padded; warning suppressed */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpadded"
#elif defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4820 )
#endif
typedef struct
{
    FILE * stream;
    solog_level_t level;
    unsigned features;
} solog_config_t;
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning( pop )
#endif

extern solog_config_t solog_config;

/* SOLOG MACRO */
/* Feel free to shorten the macro itself to LOG, the rest should remain
 * prefixed / namespaced.
 */
#define SOLOG( lvl, ... ) do { if ( SOLOG_LVL_##lvl >= solog_config.level ) { solog( SOLOG_LVL_##lvl, __FILE__, __func__, __LINE__, __VA_ARGS__ ); } else { (void)0; } } while ( 0 )

/* The worker function called by the macro */
void solog( solog_level_t level, char const * file, char const * func, int line, char const * fmt, ... );

#ifndef SOLOG_ALLOCA_MAX
#define SOLOG_ALLOCA_MAX 1024
#endif

/* MEMORY ALLOCATION */
/* solog_alloca(), solog_malloc(), solog_free() */
#include <stdlib.h>
#ifndef solog_malloc
#define solog_malloc( sz ) malloc( sz )
#endif
#ifndef solog_free
#define solog_free( ptr ) free( ptr )
#endif
#ifndef solog_alloca
#if defined( __GNUC__ ) || defined( __clang__ )
#define solog_alloca( sz ) __builtin_alloca( sz )
#elif defined( _WIN32 ) || defined( _MSC_VER )
#include <malloc.h>
#define solog_alloca( sz ) _alloca( sz )
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#define solog_alloca( sz ) alloca( sz )
#else
#include <alloca.h>
#define solog_alloca( sz ) alloca( sz )
#endif
#endif

typedef union
{
    int canary;
    /* maxalign_t not available pre-C11 so we make do */
    long double ld;
    long long ll;
    void * ptr;
    void (*fptr)(void);
} solog_maxalign_t;

static inline void * solog_add_canary( void * ptr, int canary )
{
    solog_maxalign_t * memhdr = (solog_maxalign_t *)ptr;
    memhdr->canary = canary;
    return (void *)( ++memhdr );
}

#define solog_malloca( sz ) \
    ( ( (size_t)(sz) <= (SOLOG_ALLOCA_MAX - sizeof( solog_maxalign_t )) ) \
        ? solog_add_canary( solog_alloca( (size_t)(sz) + sizeof( solog_maxalign_t ) ), 0x00 ) \
        : solog_add_canary( solog_malloc( (size_t)(sz) + sizeof( solog_maxalign_t ) ), 0x01 ) )

static inline void solog_freea( void * ptr )
{
    if ( ptr )
    {
        solog_maxalign_t * memhdr = (solog_maxalign_t *)ptr;
        --memhdr;

        if ( memhdr->canary == 0x01 )
        {
            solog_free( (void *)memhdr );
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif

/* ---------------------------------------------------------------------- */
/* End of the header part                                                 */
/* ---------------------------------------------------------------------- */

/* ---------------------------------------------------------------------- */
/* Begin of the implementation part; see usage at top of file / Readme.md */
/* ---------------------------------------------------------------------- */

/* COMPILE TIME CONFIGURATION, DEFAULT == ULTRALIGHT */
#ifdef SOLOG_IMPLEMENTATION
#if SOLOG_IMPLEMENTATION + 0 == 0
#undef SOLOG_IMPLEMENTATION
#define SOLOG_IMPLEMENTATION 0
#endif

#include <stdarg.h>
#include <string.h>
#if ( SOLOG_IMPLEMENTATION ) & ( SOLOG_FEATURE_TIME | SOLOG_FEATURE_DATE )
#include <time.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* RUN TIME CONFIGURATION, INIT */
solog_config_t solog_config = {
    NULL,
    SOLOG_LVL_INFO,
    SOLOG_IMPLEMENTATION
};

/* Suppressed warnings:
 * -Wformat-nonliteral
 *   We are passing the format string by variable; this is a potential
 *   security risk if that variable is filled by user input (which it is
 *   not, in our case).
 * -Wunsafe-buffer-usage
 *   A C++-based warning, triggered whenever pointer-and-index access is
 *   being done. Which is bad style in C++, but unavoidable in C.
 * C5045
 *   Purely informational warning that /Qspectre would add mitigation
 *   code. (Triggered by range-checking an index by comparison.)
 */
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#elif defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 5045 )
#endif

/* WORKER FUNCTION */
void solog( solog_level_t level, char const * file, char const * func, int line, char const * fmt, ... )
{
    /* Step 1: Calculate the max. space required to hold the header plus
     * the user's format string ('fmt_cmpl').
     * This depends on the features selected, plus the length of the user's
     * format string, plus a newline.
    */

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_COLOR
/* max. length cols_on / cols_off */
#define SOLOG_SZ_COLOR_ON 8
#define SOLOG_SZ_COLOR_OFF 8
    static char const * const solog_cols_on[] = {
        "\033[90m",
        "",
        "\033[1m",
        "\033[1;33m",
        "\033[1;31m",
        "\033[41;97m",
        "\033[7m"
    };

    static char const * const solog_cols_off[] = {
        "\033[39m",
        "",
        "\033[22m",
        "\033[39;22m",
        "\033[39;22m",
        "\033[49;39m",
        "\033[27m"
    };
#else
#define SOLOG_SZ_COLOR_ON 0
#define SOLOG_SZ_COLOR_OFF 0
#endif

#if ( SOLOG_IMPLEMENTATION ) & ( SOLOG_FEATURE_TIME | SOLOG_FEATURE_DATE )
#define SOLOG_SZ_TIME 11
    time_t now = time( NULL );
    struct tm * tm = localtime( &now );
#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_DATE
#define SOLOG_SZ_DATE 11
#else
#define SOLOG_SZ_DATE 0
#endif
#else
#define SOLOG_SZ_TIME 0
#define SOLOG_SZ_DATE 0
#endif

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_LEVEL
/* max. level length + ' | ' */
#define SOLOG_SZ_LEVEL 8
    static char const * const solog_hdrs[] = {
        "TRACE",
        "DEBUG",
        " INFO",
        " WARN",
        "  ERR",
        " FAIL",
        "ILLGL"
    };
#else
#define SOLOG_SZ_LEVEL 0
#endif

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_FILE
#ifndef SOLOG_SZ_FILE
#define SOLOG_SZ_FILE 16
#endif
#else
#define SOLOG_SZ_FILE 0
#endif
#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_FUNC
#ifndef SOLOG_SZ_FUNC
#define SOLOG_SZ_FUNC 16
#endif
#else
#define SOLOG_SZ_FUNC 0
#endif
#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_LINE
#ifndef SOLOG_SZ_LINE
#define SOLOG_SZ_LINE 7
#endif
#else
#define SOLOG_SZ_LINE 0
#endif

#define SOLOG_SZ_FMT ( \
        SOLOG_SZ_COLOR_ON + \
        SOLOG_SZ_COLOR_OFF + \
        SOLOG_SZ_DATE + \
        SOLOG_SZ_TIME + \
        SOLOG_SZ_LEVEL + \
        SOLOG_SZ_FILE + \
        SOLOG_SZ_FUNC + \
        SOLOG_SZ_LINE + \
        2 )

    /* Step 2: Allocate header memory */

    size_t fmt_len = SOLOG_SZ_FMT + strlen( fmt );

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Walloca"
#endif

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_MALLOC
    char * fmt_cmpl = (char *)solog_malloca( ( ! ( solog_config.features & SOLOG_FEATURE_MALLOC ) && ( fmt_len > SOLOG_ALLOCA_MAX ) ) ? SOLOG_ALLOCA_MAX : fmt_len );
#else
    char * fmt_cmpl = (char *)solog_alloca( fmt_len > SOLOG_ALLOCA_MAX ? SOLOG_ALLOCA_MAX : fmt_len );
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    char * fptr = fmt_cmpl;

    /* Step 3: Range-clamp the log level */

#if ( SOLOG_IMPLEMENTATION ) & ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR )
    int const lvl = ( ( level >= SOLOG_LVL_TRACE ) && ( level <= SOLOG_LVL_FAIL ) ) ? (int)level : SOLOG_LVL_FAIL + 1;
#endif

    /* Step 4: Final declarations */

    FILE * stream = ( solog_config.stream == NULL ) ? stderr : solog_config.stream;
    va_list ap;

    va_start( ap, fmt );
    (void)file;
    (void)func;
    (void)line;
#if ! ( ( SOLOG_IMPLEMENTATION ) & ( SOLOG_FEATURE_LEVEL | SOLOG_FEATURE_COLOR ) )
    (void)level;
#endif

    /* Step 5: Assemble the header */

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_COLOR
    if ( solog_config.features & SOLOG_FEATURE_COLOR )
    {
        fptr += sprintf( fptr, "%.*s", SOLOG_SZ_COLOR_ON, solog_cols_on[ lvl ] );
    }
#endif

#if ( SOLOG_IMPLEMENTATION ) & ( SOLOG_FEATURE_TIME | SOLOG_FEATURE_DATE )
#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_DATE
    if ( solog_config.features & SOLOG_FEATURE_DATE )
    {
        fptr += strftime( fptr, SOLOG_SZ_DATE + 1, "%Y-%m-%d ", tm );
    }
#endif
    if ( solog_config.features & ( SOLOG_FEATURE_TIME | SOLOG_FEATURE_DATE ) )
    {
        fptr += strftime( fptr, SOLOG_SZ_TIME + 1, "%H:%M:%S | ", tm );
    }
#endif

#if SOLOG_IMPLEMENTATION & SOLOG_FEATURE_LEVEL
    if ( solog_config.features & SOLOG_FEATURE_LEVEL )
    {
        fptr += sprintf( fptr, "%.*s | ", SOLOG_SZ_LEVEL - 3, solog_hdrs[ lvl ] );
    }
#endif

#if SOLOG_IMPLEMENTATION & SOLOG_FEATURE_FILE
    if ( solog_config.features & SOLOG_FEATURE_FILE )
    {
        fptr += sprintf( fptr, "%*.*s | ", SOLOG_SZ_FILE - 3, SOLOG_SZ_FILE - 3, file );
    }
#endif

#if SOLOG_IMPLEMENTATION & SOLOG_FEATURE_FUNC
    if ( solog_config.features & SOLOG_FEATURE_FUNC )
    {
        fptr += sprintf( fptr, "%*.*s | ", SOLOG_SZ_FUNC - 3, SOLOG_SZ_FUNC - 3, func );
    }
#endif

#if SOLOG_IMPLEMENTATION & SOLOG_FEATURE_LINE
    if ( solog_config.features & SOLOG_FEATURE_LINE )
    {
        fptr += sprintf( fptr, "%*d | ", SOLOG_SZ_LINE - 3, line );
    }
#endif

    /* Step 6: Append the user's format string to the header */

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_MALLOC
    if ( solog_config.features & SOLOG_FEATURE_MALLOC )
    {
        fptr += sprintf( fptr, "%s", fmt );
    }
    else
    {
        fptr += sprintf( fptr, "%.*s", SOLOG_ALLOCA_MAX - (int)( ( fptr + SOLOG_SZ_COLOR_OFF + 2 ) - fmt_cmpl ), fmt );
    }
#else
    fptr += sprintf( fptr, "%.*s", SOLOG_ALLOCA_MAX - (int)( ( fptr + SOLOG_SZ_COLOR_OFF + 2 ) - fmt_cmpl ), fmt );
#endif

    /* Step 6a: Append color-off sequence if required */

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_COLOR
    if ( solog_config.features & SOLOG_FEATURE_COLOR )
    {
        fptr += sprintf( fptr, "%.*s", SOLOG_SZ_COLOR_OFF, solog_cols_off[ lvl ] );
    }
#endif

    *fptr++ = '\n';
    *fptr = '\0';

    /* Step 7: Print the log message using our header-plus-format-string. */

    vfprintf( stream, fmt_cmpl, ap );
    va_end( ap );

    /* Step 8: Clean up. */

#if ( SOLOG_IMPLEMENTATION ) & SOLOG_FEATURE_MALLOC
    solog_freea( fmt_cmpl );
#endif
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning( pop )
#endif

#ifdef __cplusplus
}
#endif

#endif
