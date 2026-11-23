#define _POSIX_C_SOURCE 200809L

#include "support/greatest.h"
#include "context.h"
#include "options.h"
#include "utils.h"

#include <sys/wait.h>
#include <unistd.h>

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

static int run_expect_exit( int argc, char * argv[] )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        ctx_init();
        parse_options( argc, argv );
        ctx_cleanup();
        _exit( 0 );
    }

    if ( pid > 0 )
    {
        int status = 0;

        if ( waitpid( pid, &status, 0 ) == pid && WIFEXITED( status ) )
        {
            return WEXITSTATUS( status );
        }
    }

    return -1;
}

TEST test_target_mode_same( void )
{
    char * argv_long[] = {
        "x86_64-linux-gnu-gcc",
        "-Wx,target=x86_64-linux-gnu",
        "-Wx,mode=gcc"
    };
    char * argv_short[] = {
        "aarch64-linux-gnu-clang",
        "-Wx,t=aarch64-linux-gnu",
        "-Wx,m=clang"
    };

    ctx_init();
    parse_options( 3, argv_long );
    ASSERT_STR_EQ( "x86_64-linux-gnu", first( &g_ctx.target ) );
    ASSERT_STR_EQ( "gcc", first( &g_ctx.mode ) );
    ctx_cleanup();

    ctx_init();
    parse_options( 3, argv_short );
    ASSERT_STR_EQ( "aarch64-linux-gnu", first( &g_ctx.target ) );
    ASSERT_STR_EQ( "clang", first( &g_ctx.mode ) );
    ctx_cleanup();

    PASS();
}

TEST test_target_mode_conflict( void )
{
    char * argv_diff_target[] = {
        "x86_64-linux-gnu-gcc",
        "-Wx,target=aarch64-linux-gnu"
    };
    char * argv_diff_mode[] = {
        "x86_64-linux-gnu-gcc",
        "-Wx,mode=clang"
    };
    char * argv_repeat_target[] = {
        "crosscc",
        "-Wx,target=x86_64-linux-gnu",
        "-Wx,target=aarch64-linux-gnu"
    };

    ASSERT_EQ( EXIT_FAILURE, run_expect_exit( 2, argv_diff_target ) );
    ASSERT_EQ( EXIT_FAILURE, run_expect_exit( 2, argv_diff_mode ) );
    ASSERT_EQ( EXIT_FAILURE, run_expect_exit( 3, argv_repeat_target ) );

    PASS();
}

SUITE( options_suite )
{
    RUN_TEST( test_parse_options );
    RUN_TEST( test_exe_name );
    RUN_TEST( test_target_mode_same );
    RUN_TEST( test_target_mode_conflict );
}
