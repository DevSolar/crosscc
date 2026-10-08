#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "support/greatest.h"
#include "utils.h"

TEST test_stricmp( void )
{
    /* Equal */
    ASSERT_EQ( 0, stricmp( "hello", "HELLO" ) );
    ASSERT_EQ( 0, stricmp( "foo", "foo" ) );
    ASSERT_EQ( 0, stricmp( "BAR", "BAR" ) );
    ASSERT_EQ( 0, stricmp( "", "" ) );

    /* Not Equal */
    ASSERT( stricmp( "abc", "abd" ) < 0 );
    ASSERT( stricmp( "abd", "abc" ) > 0 );
    ASSERT( stricmp( "ab", "abc" ) < 0 );
    ASSERT( stricmp( "abc", "ab" ) > 0 );
    ASSERT( stricmp( "abc", "" ) > 0 );
    ASSERT( stricmp( "", "abc" ) < 0 );

    PASS();
}

TEST test_strnicmp( void )
{
    /* Equal */
    ASSERT_EQ( 0, strnicmp( "hello world", "HELLO BAZ", 5 ) );
    ASSERT_EQ( 0, strnicmp( "foo", "FOO", 3 ) );
    ASSERT_EQ( 0, strnicmp( "abc", "def", 0 ) );

    /* Not Equal */
    ASSERT( strnicmp( "abcd", "abce", 4 ) < 0 );
    ASSERT( strnicmp( "abce", "abcd", 4 ) > 0 );
    ASSERT( strnicmp( "ab", "abc", 4 ) < 0 );
    ASSERT( strnicmp( "abc", "ab", 4 ) > 0 );
    ASSERT( strnicmp( "abc", "", 4 ) > 0 );
    ASSERT( strnicmp( "", "abc", 4 ) < 0 );

    PASS();
}

TEST test_strvicmp( void )
{
    /* Numeric comparison */
    ASSERT( strvicmp( "clang-11", "clang-9" ) > 0 );
    ASSERT( strvicmp( "clang-9", "clang-11" ) < 0 );
    ASSERT( strvicmp( "CLANG-11", "clang-9" ) > 0 );
    ASSERT_EQ( 0, strvicmp( "clang-11", "CLANG-11" ) );
    ASSERT( strvicmp( "clang-11.2", "clang-11.3" ) < 0 );
    ASSERT( strvicmp( "clang-11.3", "clang-11.2" ) > 0 );
    ASSERT_EQ( 0, strvicmp( "", "" ) );

    PASS();
}

TEST test_file_readable( void )
{
    ASSERT_EQ( false, file_readable( NULL ) );
    ASSERT_EQ( false, file_readable( "" ) );
    ASSERT_EQ( false, file_readable( "non_existent_file" ) );
    ASSERT_EQ( false, file_readable( "src" ) );
    ASSERT( file_readable( "Makefile" ) );
    ASSERT( file_readable( "src/utils.c" ) );

    PASS();
}

TEST test_dir_readable( void )
{
    ASSERT_EQ( false, dir_readable( NULL ) );
    ASSERT_EQ( false, dir_readable( "" ) );
    ASSERT_EQ( false, dir_readable( "non_existent_dir" ) );
    ASSERT_EQ( false, dir_readable( "Makefile" ) );
    ASSERT( dir_readable( "src" ) );
    ASSERT( dir_readable( "src/support" ) );

    PASS();
}

TEST test_is_newer( void )
{
    FILE * f1;
    FILE * f2;

    ASSERT_EQ( false, is_newer( NULL, NULL ) );
    ASSERT_EQ( false, is_newer( "Makefile", NULL ) );
    ASSERT_EQ( false, is_newer( NULL, "Makefile" ) );
    ASSERT_EQ( false, is_newer( "", "" ) );
    ASSERT_EQ( false, is_newer( "non_existent_1", "non_existent_2" ) );
    ASSERT_EQ( false, is_newer( "non_existent_1", "Makefile" ) );

    ASSERT_EQ( true, is_newer( "Makefile", "non_existent_rhs" ) );
    ASSERT_EQ( false, is_newer( "Makefile", "Makefile" ) );

    f1 = fopen( "tmp_test_older.tmp", "w" );
    ASSERT( f1 != NULL );
    fputs( "a", f1 );
    fclose( f1 );

    f2 = fopen( "tmp_test_newer.tmp", "w" );
    ASSERT( f2 != NULL );
    fputs( "b", f2 );
    fclose( f2 );

    ASSERT_EQ( true, is_newer( "tmp_test_newer.tmp", "non_existent_target" ) );
    ASSERT_EQ( false, is_newer( "tmp_test_older.tmp", "tmp_test_older.tmp" ) );

    remove( "tmp_test_older.tmp" );
    remove( "tmp_test_newer.tmp" );

    PASS();
}

TEST test_rtrim( void )
{
    char buf1[] = "hello   ";
    char buf2[] = "  world\t\n ";
    char buf3[] = "no_trailing";
    char buf4[] = "   ";
    char buf5[] = "";
    char * buf6 = NULL;

    rtrim( buf1 );
    ASSERT_STR_EQ( "hello", buf1 );

    rtrim( buf2 );
    ASSERT_STR_EQ( "  world", buf2 );

    rtrim( buf3 );
    ASSERT_STR_EQ( "no_trailing", buf3 );

    rtrim( buf4 );
    ASSERT_STR_EQ( "", buf4 );

    rtrim( buf5 );
    ASSERT_STR_EQ( "", buf5 );

    rtrim( buf6 );
    ASSERT( buf6 == NULL );

    PASS();
}

TEST test_rtrim_str( void )
{
    string s1;
    string s2;
    string s3;
    string * s4 = NULL;

    init( &s1 );
    push_fmt( &s1, "hello   " );
    rtrim_str( &s1 );
    ASSERT_STR_EQ( "hello", first( &s1 ) );
    cleanup( &s1 );

    init( &s2 );
    push_fmt( &s2, "   " );
    rtrim_str( &s2 );
    ASSERT_EQ( 0, size( &s2 ) );
    cleanup( &s2 );

    init( &s3 );
    /*push_fmt( &s3, "" ); // segfaults, known bug in CC */
    rtrim_str( &s3 );
    ASSERT_EQ( 0, size( &s3 ) );
    cleanup( &s3 );

    rtrim_str( s4 );
    ASSERT( s4 == NULL );

    PASS();
}

TEST test_ltrim( void )
{
    char buf1[] = "   hello";
    char buf2[] = " \t\n world  ";
    char buf3[] = "no_leading";
    char buf4[] = "   ";
    char buf5[] = "";
    char * buf6 = NULL;

    ASSERT_STR_EQ( "hello", ltrim( buf1 ) );
    ASSERT_STR_EQ( "world  ", ltrim( buf2 ) );
    ASSERT_STR_EQ( "no_leading", ltrim( buf3 ) );
    ASSERT_STR_EQ( "", ltrim( buf4 ) );
    ASSERT_STR_EQ( "", ltrim( buf5 ) );
    ASSERT( ltrim( buf6 ) == NULL );

    PASS();
}

TEST test_ltrim_str( void )
{
    string s1;
    string s2;
    string s3;
    string * s4 = NULL;

    init( &s1 );
    push_fmt( &s1, "   hello" );
    ltrim_str( &s1 );
    ASSERT_STR_EQ( "hello", first( &s1 ) );
    cleanup( &s1 );

    init( &s2 );
    push_fmt( &s2, "   " );
    ltrim_str( &s2 );
    ASSERT_EQ( 0, size( &s2 ) );
    cleanup( &s2 );

    init( &s3 );
    /*push_fmt( &s3, "" ); // segfaults, known bug in CC */
    rtrim_str( &s3 );
    cleanup( &s3 );

    ltrim_str( s4 );
    ASSERT( s4 == NULL );

    PASS();
}

TEST test_get_line( void )
{
    char const content[] = "line 1\nline 2\nline 3\nline 4 \\\n  line 5\nline 6 ";
    FILE * fh = fmemopen( (void *)content, strlen( content ), "r" );
    string line;
    int lineno = 0;
    int rc;

    ASSERT( fh != NULL );
    init( &line );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 1, rc );
    ASSERT_STR_EQ( "line 1", first( &line ) );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 2, rc );
    ASSERT_STR_EQ( "line 2", first( &line ) );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 3, rc );
    ASSERT_STR_EQ( "line 3", first( &line ) );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 5, rc );
    ASSERT_STR_EQ( "line 4 line 5", first( &line ) );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 6, rc );
    ASSERT_STR_EQ( "line 6", first( &line ) );

    rc = get_line( fh, &line, &lineno );
    ASSERT_EQ( 0, rc );

    ASSERT_EQ( -1, get_line( NULL, &line, &lineno ) );

    cleanup( &line );
    fclose( fh );

    PASS();
}

/* Persistent userdata type */
typedef struct
{
    size_t max_count;
    vec( string ) tokens;
} userdata_t;

/* Callback function, called per token */
static bool callback_func( string * token, void * udata )
{
    userdata_t * userdata = (userdata_t *)udata;
    string s;
    init( &s );

    push_fmt( &s, first( token ) );
    push( &userdata->tokens, s );

    return ! ( userdata->max_count > 0 && size( &userdata->tokens ) >= userdata->max_count );
}

/* Cleanup */
static void cleanup_userdata( userdata_t * userdata )
{
    for_each( &userdata->tokens, token )
    {
        cleanup( token );
    }

    cleanup( &userdata->tokens );
}

TEST test_foreach( void )
{
    userdata_t userdata;

    /* 1. Standard unquoted token list */
    init( &userdata.tokens );
    userdata.max_count = 0;

    ASSERT_EQ( 4, foreach( "foo,bar:baz;qux", ":;,", callback_func, &userdata ) );
    ASSERT_EQ( 4, size( &userdata.tokens ) );

    {
        string * t0 = get( &userdata.tokens, 0 );
        string * t1 = get( &userdata.tokens, 1 );
        string * t2 = get( &userdata.tokens, 2 );
        string * t3 = get( &userdata.tokens, 3 );
        ASSERT_STR_EQ( "foo", first( t0 ) );
        ASSERT_STR_EQ( "bar", first( t1 ) );
        ASSERT_STR_EQ( "baz", first( t2 ) );
        ASSERT_STR_EQ( "qux", first( t3 ) );
    }

    cleanup_userdata( &userdata );

    /* 2. Quoted tokens and spaces */
    init( &userdata.tokens );
    userdata.max_count = 0;

    ASSERT_EQ( 2, foreach( "'hello world', \"foo bar\"", ", ", callback_func, &userdata ) );
    ASSERT_EQ( 2, size( &userdata.tokens ) );

    {
        string * t0 = get( &userdata.tokens, 0 );
        string * t1 = get( &userdata.tokens, 1 );
        ASSERT_STR_EQ( "hello world", first( t0 ) );
        ASSERT_STR_EQ( "foo bar", first( t1 ) );
    }

    cleanup_userdata( &userdata );

    /* 3. Custom delimiters */
    init( &userdata.tokens );
    userdata.max_count = 0;

    ASSERT_EQ( 3, foreach( "one|two|three", "|", callback_func, &userdata ) );
    ASSERT_EQ( 3, size( &userdata.tokens ) );

    {
        string * t0 = get( &userdata.tokens, 0 );
        string * t1 = get( &userdata.tokens, 1 );
        string * t2 = get( &userdata.tokens, 2 );
        ASSERT_STR_EQ( "one", first( t0 ) );
        ASSERT_STR_EQ( "two", first( t1 ) );
        ASSERT_STR_EQ( "three", first( t2 ) );
    }

    cleanup_userdata( &userdata );

    /* 4. Early termination (callback returns false) */
    init( &userdata.tokens );
    userdata.max_count = 2;

    ASSERT_EQ( 2, foreach( "a,b,c,d", ",", callback_func, &userdata ) );
    ASSERT_EQ( 2, size( &userdata.tokens ) );

    {
        string * t0 = get( &userdata.tokens, 0 );
        string * t1 = get( &userdata.tokens, 1 );
        ASSERT_STR_EQ( "a", first( t0 ) );
        ASSERT_STR_EQ( "b", first( t1 ) );
    }

    cleanup_userdata( &userdata );

    /* 5. Edge cases: empty/null list, whitespace, trailing delimiters */
    init( &userdata.tokens );
    userdata.max_count = 0;

    ASSERT_EQ( 0, foreach( NULL, ",", callback_func, &userdata ) );
    ASSERT_EQ( 0, foreach( "", ",", callback_func, &userdata ) );
    ASSERT_EQ( 0, foreach( "   ", " ", callback_func, &userdata ) );
    ASSERT_EQ( 2, foreach( "foo,bar,", ",", callback_func, &userdata ) );

    cleanup_userdata( &userdata );

    /* 6. Error cases: NULL delims, unmatched quotes, partially quoted tokens */
    init( &userdata.tokens );
    userdata.max_count = 0;

    ASSERT_EQ( -1, foreach( "foo,bar", NULL, callback_func, &userdata ) );
    ASSERT_EQ( -1, foreach( "'unmatched quote", ",", callback_func, &userdata ) );
    ASSERT_EQ( -1, foreach( "'partially'quoted", ",", callback_func, &userdata ) );

    cleanup_userdata( &userdata );

    PASS();
}

TEST test_find_first( void )
{
    vec( candidate_t ) candidates;
    init( &candidates );

    /* Test 1: Verbatim candidate path */
    {
        string res;
        candidate_t c = { NULL, "Makefile" };
        push( &candidates, c );
        res = find_first( candidates );
        ASSERT_STR_EQ( "Makefile", first( &res ) );
        cleanup( &res );
        clear( &candidates );
    }

    /* Test 2: Environment variable expansion (PATH with subpath) */
    {
        string res;
        candidate_t c = { "PATH", "ls" };
        push( &candidates, c );
        res = find_first( candidates );
        ASSERT( size( &res ) > 0 );
        cleanup( &res );
        clear( &candidates );
    }

    /* Test 3: Null subpath with environment variable */
    {
        string res;
        candidate_t c = { "PATH", NULL };
        push( &candidates, c );
        res = find_first( candidates );
        /* Should handle NULL subpath gracefully without crashing or formatting 0x0 */
        cleanup( &res );
        clear( &candidates );
    }

    cleanup( &candidates );
    PASS();
}

TEST test_spantol( void )
{
    char const * endptr;
    strspan_t span;
    char overflow[] = "9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999_";
    char non_null_term[] = "12345extra";
    char whitespace_test[] = " \n\v\t\f789";
    char spaces[] = "   ";
    char plus[] = "+";
    char minus[] = "-";

    errno = 0;

    /* basic functionality */
    span = (strspan_t){ "123", &"123"[3] };
    ASSERT_EQ( 123, spantol( &span, NULL ) );

    /* plus sign not accepted */
    span = (strspan_t){ "+456", &"+456"[4] };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    /* base 10 interpretation of leading zeros (no octal) */
    span = (strspan_t){ "016", &"016"[3] };
    ASSERT_EQ( 16, spantol( &span, NULL ) );

    /* no hexadecimal autodetection: stops at 'x' */
    span = (strspan_t){ "0xFF", &"0xFF"[4] };
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == &"0xFF"[1] );

    /* errno should still be 0 */
    ASSERT_EQ( 0, errno );

    /* correctly decoding zero */
    span = (strspan_t){ "0", &"0"[1] };
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.end );
    ASSERT_EQ( 0, errno );

    /* overflowing subject sequence returns LONG_MAX and sets ERANGE */
    span = (strspan_t){ overflow, overflow + strlen( overflow ) };
    ASSERT_EQ( LONG_MAX, spantol( &span, &endptr ) );
    ASSERT_EQ( ERANGE, errno );
    ASSERT( *endptr == '_' );
    ASSERT_EQ( strlen( overflow ) - 1, (size_t)( endptr - overflow ) );

    /* leading whitespace not skipped (conversion fails) */
    span = (strspan_t){ whitespace_test, whitespace_test + 8 };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    /* negative numbers not accepted (positive numbers only) */
    span = (strspan_t){ "-123", &"-123"[4] };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    /* non-null-terminated span (stops at span.end) */
    span = (strspan_t){ non_null_term, non_null_term + 5 };
    ASSERT_EQ( 12345, spantol( &span, &endptr ) );
    ASSERT( endptr == non_null_term + 5 );

    /* boundary tests */
    errno = 0;
#if LONG_MAX >> 30 == 1
    span = (strspan_t){ "2147483647", &"2147483647"[10] };
    ASSERT_EQ( 0x7fffffffL, spantol( &span, NULL ) );
    ASSERT_EQ( 0, errno );

    errno = 0;
    span = (strspan_t){ "2147483648", &"2147483648"[10] };
    ASSERT_EQ( LONG_MAX, spantol( &span, NULL ) );
    ASSERT_EQ( ERANGE, errno );
#elif LONG_MAX >> 62 == 1
    span = (strspan_t){ "9223372036854775807", &"9223372036854775807"[19] };
    ASSERT_EQ( 0x7fffffffffffffffL, spantol( &span, NULL ) );
    ASSERT_EQ( 0, errno );

    errno = 0;
    span = (strspan_t){ "9223372036854775808", &"9223372036854775808"[19] };
    ASSERT_EQ( LONG_MAX, spantol( &span, NULL ) );
    ASSERT_EQ( ERANGE, errno );
#endif

    /* empty / invalid spans */
    span = (strspan_t){ "", "" };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    span = (strspan_t){ spaces, spaces + 3 };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    span = (strspan_t){ plus, plus + 1 };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    span = (strspan_t){ minus, minus + 1 };
    endptr = NULL;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == span.begin );

    /* NULL span / pointers */
    endptr = (char const *)0x1;
    ASSERT_EQ( 0, spantol( NULL, &endptr ) );
    ASSERT( endptr == NULL );

    span = (strspan_t){ NULL, NULL };
    endptr = (char const *)0x1;
    ASSERT_EQ( 0, spantol( &span, &endptr ) );
    ASSERT( endptr == NULL );

    PASS();
}

SUITE( utils_suite )
{
    RUN_TEST( test_stricmp );
    RUN_TEST( test_strnicmp );
    RUN_TEST( test_strvicmp );
    RUN_TEST( test_file_readable );
    RUN_TEST( test_dir_readable );
    RUN_TEST( test_is_newer );
    RUN_TEST( test_rtrim );
    RUN_TEST( test_rtrim_str );
    RUN_TEST( test_ltrim );
    RUN_TEST( test_ltrim_str );
    RUN_TEST( test_get_line );
    RUN_TEST( test_foreach );
    RUN_TEST( test_find_first );
    RUN_TEST( test_spantol );
}
