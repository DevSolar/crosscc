#include "support/greatest.h"
#include "context.h"
#include "options.h"
#include "utils.h"

TEST test_parse_options( void )
{
    string * p0;
    string * p1;
    string * p2;
    char * argv[] = {
        "crosscc",
        "-Wx,v=2",
        "-Wx,dryrun",
        "-Wx,build=release",
        "-Wx,target=x86_64-linux-gnu",
        "main.c",
        "-o",
        "main"
    };
    int argc = (int)( sizeof( argv ) / sizeof( argv[0] ) );

    ctx_init();
    parse_options( argc, argv );

    p0 = get( &g_ctx.passthrough, 0 );
    p1 = get( &g_ctx.passthrough, 1 );
    p2 = get( &g_ctx.passthrough, 2 );

    ASSERT_STR_EQ( "crosscc", first( &g_ctx.command ) );
    ASSERT_EQ( 2L, g_ctx.verbose );
    ASSERT_EQ( true, g_ctx.dryrun );
    ASSERT_STR_EQ( "release", first( &g_ctx.build ) );
    ASSERT_STR_EQ( "x86_64-linux-gnu", first( &g_ctx.target ) );
    ASSERT_EQ( 3, size( &g_ctx.passthrough ) );
    ASSERT( p0 != NULL );
    ASSERT_STR_EQ( "main.c", first( p0 ) );
    ASSERT( p1 != NULL );
    ASSERT_STR_EQ( "-o", first( p1 ) );
    ASSERT( p2 != NULL );
    ASSERT_STR_EQ( "main", first( p2 ) );

    ctx_cleanup();
    PASS();
}

TEST test_exe_name( void )
{
    char * argv_posix[] = { "/usr/local/bin/crcc" };
    char * argv_win[] = { "C:\\Program Files\\crosscc\\crcc.EXE" };
    char * argv_mixed[] = { "C:\\tools/bin\\mycc.eXe" };
    char * argv_dotexe[] = { "/bin/.exe" };
    char * argv_trailing[] = { "/usr/bin/" };
    char * argv_empty[] = { "" };
    char * argv_null[] = { NULL };

    ctx_init();
    parse_options( 1, argv_posix );
    ASSERT_STR_EQ( "crcc", first( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_win );
    ASSERT_STR_EQ( "crcc", first( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_mixed );
    ASSERT_STR_EQ( "mycc", first( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_dotexe );
    ASSERT_STR_EQ( ".exe", first( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_trailing );
    ASSERT_EQ( 0, size( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_empty );
    ASSERT_EQ( 0, size( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 1, argv_null );
    ASSERT_EQ( 0, size( &g_ctx.command ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 0, NULL );
    ASSERT_EQ( 0, size( &g_ctx.command ) );
    ctx_cleanup();

    PASS();
}

SUITE( options_suite )
{
    RUN_TEST( test_parse_options );
    RUN_TEST( test_exe_name );
}
