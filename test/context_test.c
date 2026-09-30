#include "support/greatest.h"
#include "context.h"

TEST test_context_init_and_cleanup( void )
{
    str( char ) p1;
    str( char ) k1, v1;

    ctx_init();

    push_fmt( &g_ctx.config, "release" );
    push_fmt( &g_ctx.target, "x86_64" );
    g_ctx.sanitize = true;
    g_ctx.verbose = 3;

    init( &p1 );
    push_fmt( &p1, "-Wall" );
    push( &g_ctx.passthrough, p1 );

    init( &k1 );
    init( &v1 );
    push_fmt( &k1, "CC" );
    push_fmt( &v1, "clang" );
    insert( &g_ctx.settings, k1, v1 );

    ASSERT_EQ( 1, size( &g_ctx.passthrough ) );
    ASSERT_EQ( 1, size( &g_ctx.settings ) );

    ctx_cleanup();

    ASSERT_EQ( 0, size( &g_ctx.config ) );
    ASSERT_EQ( 0, size( &g_ctx.target ) );
    ASSERT_EQ( 0, size( &g_ctx.passthrough ) );
    ASSERT_EQ( 0, size( &g_ctx.settings ) );
    ASSERT_EQ( false, g_ctx.sanitize );
    ASSERT_EQ( 0, g_ctx.verbose );

    PASS();
}

TEST test_context_debug_output( void )
{
    ctx_init();
    dump();
    ctx_cleanup();
    PASS();
}

SUITE( context_suite )
{
    RUN_TEST( test_context_init_and_cleanup );
    RUN_TEST( test_context_debug_output );
}
