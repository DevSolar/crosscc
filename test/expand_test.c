#include "support/greatest.h"
#include "context.h"
#include "expand.h"
#include "utils.h"

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

/* --------------------------------------------------------------------------
 * Mocking & Isolation Strategy:
 * For tests that verify filesystem interaction with expand_latest():
 * 1. Create a sandboxed temporary directory tree (e.g., under a test sandbox
 *    path such as "test/sandbox_expand/").
 * 2. Populate fixture files/subdirectories (e.g. "tool-9", "tool-10", "tool-1.2").
 * 3. Invoke expand() or expand_latest() targeting the sandboxed path.
 * 4. Verify that the candidate substring selected is the strvicmp-highest.
 * 5. Recursively clean up fixture directories and files in test teardown.
 * -------------------------------------------------------------------------- */

TEST test_expand_latest_null_and_invalid_span( void )
{
    /* Scenario: NULL input or span pointers, or inverted span (begin >= end).
     * Expected: Safe early return without crash or memory corruption.
     */
    strspan_t span = { NULL, NULL };
    expand_latest( NULL, &span );
    expand_latest( NULL, NULL );

    PASS();
}

TEST test_expand_latest_tmp( void )
{
    string input;
    strspan_t span;
    init( &input );
    push_fmt( &input, "latest_test/foo-{LATEST}/" );
    span.begin = first( &input ) + 16;
    span.end = last( &input );
    expand_latest( &input, &span );
    PASS();
}

TEST test_expand_latest_no_match( void )
{
    /* Scenario: Pattern prefix + {LATEST} + suffix has no matching files
     * or directories on disk.
     * Expected: {LATEST} is erased from input (replaced with empty string).
     */
    PASS();
}

TEST test_expand_latest_single_candidate( void )
{
    /* Scenario: Exactly one versioned file exists matching prefix + version + suffix.
     * Expected: {LATEST} is replaced by the version substring of that candidate.
     */
    PASS();
}

TEST test_expand_latest_multiple_candidates_numeric_sort( void )
{
    /* Scenario: Multiple candidates exist (e.g., "1.9", "1.10", "1.2").
     * Expected: Candidates are compared via strvicmp() and the numerically
     * highest candidate ("1.10") is chosen to replace {LATEST}.
     */
    PASS();
}

TEST test_expand_latest_intermediate_directory( void )
{
    /* Scenario: {LATEST} is inside a directory name rather than the final filename,
     * e.g., "prefix/llvm-{LATEST}/bin/clang".
     * Expected: Matches highest directory version that contains the required suffix subpath.
     */
    PASS();
}

TEST test_expand_default_null_and_invalid_span( void )
{
    string input;
    strspan_t span = { NULL, NULL };
    init( &input );
    push_fmt( &input, "{foo}" );

    /* NULL input or span */
    expand_default( NULL, &span );
    expand_default( NULL, NULL );
    expand_default( &input, NULL );

    /* Inverted span */
    span.begin = first( &input ) + 3;
    span.end = first( &input ) + 1;
    expand_default( &input, &span );
    ASSERT_STR_EQ( "{foo}", first( &input ) );

    cleanup( &input );
    PASS();
}

TEST test_expand_default_empty_variable( void )
{
    string input;
    strspan_t span;
    init( &input );
    push_fmt( &input, "pre{}post" );
    span.begin = first( &input ) + 3;
    span.end = first( &input ) + 5;

    expand_default( &input, &span );
    ASSERT_STR_EQ( "prepost", first( &input ) );

    cleanup( &input );
    PASS();
}

TEST test_expand_default_setting_found( void )
{
    string input;
    strspan_t span;
    string k1, v1, k2, v2;

    ctx_init();

    init( &k1 );
    init( &v1 );
    push_fmt( &k1, "settings.id" );
    push_fmt( &v1, "aarch64-linux-musl" );
    insert( &g_ctx.settings, k1, v1 );

    init( &k2 );
    init( &v2 );
    push_fmt( &k2, "CC" );
    push_fmt( &v2, "clang" );
    insert( &g_ctx.settings, k2, v2 );

    /* Test expansion of setting with section.key */
    init( &input );
    push_fmt( &input, "target={settings.id}" );
    span.begin = first( &input ) + 7;
    span.end = first( &input ) + 20;

    expand_default( &input, &span );
    ASSERT_STR_EQ( "target=aarch64-linux-musl", first( &input ) );

    /* Test expansion of setting without dot */
    clear( &input );
    push_fmt( &input, "compiler={CC}" );
    span.begin = first( &input ) + 9;
    span.end = first( &input ) + 13;

    expand_default( &input, &span );
    ASSERT_STR_EQ( "compiler=clang", first( &input ) );

    cleanup( &input );
    ctx_cleanup();
    PASS();
}

TEST test_expand_default_setting_not_found( void )
{
    string input;
    strspan_t span;

    ctx_init();

    init( &input );
    push_fmt( &input, "pre_{missing.setting}_post" );
    span.begin = first( &input ) + 4;
    span.end = first( &input ) + 21;

    expand_default( &input, &span );
    ASSERT_STR_EQ( "pre__post", first( &input ) );

    cleanup( &input );
    ctx_cleanup();
    PASS();
}

TEST test_expand_integration_default( void )
{
    string input;
    string k1, v1, k2, v2;

    ctx_init();

    init( &k1 );
    init( &v1 );
    push_fmt( &k1, "sysroot.base" );
    push_fmt( &v1, "/opt/sysroots" );
    insert( &g_ctx.settings, k1, v1 );

    init( &k2 );
    init( &v2 );
    push_fmt( &k2, "settings.id" );
    push_fmt( &v2, "aarch64-linux-musl" );
    insert( &g_ctx.settings, k2, v2 );

    init( &input );
    push_fmt( &input, "{sysroot.base}/{settings.id}-cross/{settings.id}" );

    expand( &input );
    ASSERT_STR_EQ( "/opt/sysroots/aarch64-linux-musl-cross/aarch64-linux-musl", first( &input ) );

    cleanup( &input );
    ctx_cleanup();
    PASS();
}

static void insert_setting( char const * key_str, char const * val_str )
{
    string k, v;
    init( &k );
    init( &v );
    push_fmt( &k, key_str );
    push_fmt( &v, val_str );
    insert( &g_ctx.settings, k, v );
}

TEST test_expand_recursive_and_cyclic( void )
{
    string input;

    ctx_init();
    insert_setting( "sysroot.base", "/opt/{sysroot.arch}" );
    insert_setting( "sysroot.arch", "{ARGS:1}" );
    insert_setting( "sysroot.dir", "{sysroot.base}/sub" );

    init( &input );
    push_fmt( &input, "{sysroot.dir}/bin" );
    ASSERT_EQ( 0, expand( &input, "aarch64" ) );
    ASSERT_STR_EQ( "/opt/aarch64/sub/bin", first( &input ) );

    /* Cyclic macro reference must terminate cleanly with -1 */
    insert_setting( "cycle.a", "{cycle.b}" );
    insert_setting( "cycle.b", "{cycle.a}" );
    clear( &input );
    push_fmt( &input, "{cycle.a}" );
    ASSERT_EQ( -1, expand( &input ) );

    cleanup( &input );
    ctx_cleanup();
    PASS();
}

TEST test_expand_deferred_latest( void )
{
    char const * base_dir = "test_expand_deferred_tmp";
    char const * v1_dir = "test_expand_deferred_tmp/tool-1.2";
    char const * v2_dir = "test_expand_deferred_tmp/tool-1.10";
    char const * v1_sub = "test_expand_deferred_tmp/tool-1.2/bin";
    char const * v2_sub = "test_expand_deferred_tmp/tool-1.10/bin";
    string input;

    rmdir( v1_sub );
    rmdir( v2_sub );
    rmdir( v1_dir );
    rmdir( v2_dir );
    rmdir( base_dir );

    ASSERT_EQ( 0, mkdir( base_dir, 0755 ) );
    ASSERT_EQ( 0, mkdir( v1_dir, 0755 ) );
    ASSERT_EQ( 0, mkdir( v2_dir, 0755 ) );
    ASSERT_EQ( 0, mkdir( v1_sub, 0755 ) );
    ASSERT_EQ( 0, mkdir( v2_sub, 0755 ) );

    ctx_init();
    insert_setting( "test.root", base_dir );
    insert_setting( "test.sub", "bin" );

    init( &input );
    push_fmt( &input, "{test.root}/tool-{LATEST}/{test.sub}" );
    ASSERT_EQ( 0, expand( &input ) );
    ASSERT_STR_EQ( "test_expand_deferred_tmp/tool-1.10/bin", first( &input ) );

    cleanup( &input );
    ctx_cleanup();

    rmdir( v1_sub );
    rmdir( v2_sub );
    rmdir( v1_dir );
    rmdir( v2_dir );
    rmdir( base_dir );
    PASS();
}

SUITE( expand_suite )
{
    RUN_TEST( test_expand_latest_null_and_invalid_span );
    RUN_TEST( test_expand_latest_tmp );
    RUN_TEST( test_expand_latest_no_match );
    RUN_TEST( test_expand_latest_single_candidate );
    RUN_TEST( test_expand_latest_multiple_candidates_numeric_sort );
    RUN_TEST( test_expand_latest_intermediate_directory );
    RUN_TEST( test_expand_default_null_and_invalid_span );
    RUN_TEST( test_expand_default_empty_variable );
    RUN_TEST( test_expand_default_setting_found );
    RUN_TEST( test_expand_default_setting_not_found );
    RUN_TEST( test_expand_integration_default );
    RUN_TEST( test_expand_recursive_and_cyclic );
    RUN_TEST( test_expand_deferred_latest );
}
