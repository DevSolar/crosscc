#define _POSIX_C_SOURCE 200809L
#include "support/greatest.h"
#include "config.h"
#include "context.h"
#include "utils.h"

#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_text_file( char const * path, char const * content )
{
    FILE * fh = fopen( path, "w" );
    if ( fh )
    {
        fputs( content, fh );
        fclose( fh );
    }
}

TEST test_load_repo_ini_and_target( void )
{
    string * val;
    string k, v;

    ctx_init();

    push_fmt( &g_ctx.config, "ini/config.ini" );
    load_ini();

    val = get_setting( "modes", "cc" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "compiler-c", first( val ) );

    val = get_setting( "sysroot", "link" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "static", first( val ) );

    /* Point settings.inifiles to repo's ini/ directory and load aarch64.ini */
    init( &k );
    init( &v );
    push_fmt( &k, "settings.inifiles" );
    push_fmt( &v, "ini" );
    insert( &g_ctx.settings, k, v );

    push_fmt( &g_ctx.target, "aarch64" );
    load_target_ini();

    val = get_setting( "settings", "name" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "aarch64", first( val ) );

    val = get_setting( "settings", "id" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "aarch64-linux-musl", first( val ) );

    val = get_setting( "debugger", "flags" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "-ex \"target remote :1234\"", first( val ) );

    val = get_setting( "sysroot", "checksum" );
    ASSERT( val != NULL );
    ASSERT_EQ( 0, size( val ) );

    ctx_cleanup();
    PASS();
}

TEST test_ini_syntax_features( void )
{
    char const * tmp_ini = "test_syntax_tmp.ini";
    string * val;

    write_text_file( tmp_ini,
        "# Top-level comment\n"
        "; Another comment\n"
        "\n"
        "global_key = global_val\n"
        "\n"
        "[  sec_one  ] # section comment\n"
        "eq_key = value_one ; inline comment\n"
        "colon_key : value_two # hash comment\n"
        "empty_key =\n"
        "single_quoted = 'hello ; world # 1' ; comment after quote\n"
        "double_quoted = \"foo : bar = 2\" # comment after quote\n"
        "continued = part1 \\\n"
        "    part2\n"
    );

    ctx_init();
    push_fmt( &g_ctx.config, tmp_ini );
    load_ini();

    val = get_setting( NULL, "global_key" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "global_val", first( val ) );

    val = get_setting( "sec_one", "eq_key" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "value_one", first( val ) );

    val = get_setting( "sec_one", "colon_key" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "value_two", first( val ) );

    val = get_setting( "sec_one", "empty_key" );
    ASSERT( val != NULL );
    ASSERT_EQ( 0, size( val ) );

    val = get_setting( "sec_one", "single_quoted" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "hello ; world # 1", first( val ) );

    val = get_setting( "sec_one", "double_quoted" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "foo : bar = 2", first( val ) );

    val = get_setting( "sec_one", "continued" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "part1 part2", first( val ) );

    ctx_cleanup();
    remove( tmp_ini );
    PASS();
}

TEST test_load_ini_precedence( void )
{
    char const * xdg_dir = "test_xdg_tmp";
    char const * xdg_sub = "test_xdg_tmp/crosscc";
    char const * xdg_ini = "test_xdg_tmp/crosscc/config.ini";
    char const * cwd_ini = "crosscc.ini";
    char const * cli_ini = "test_cli_tmp.ini";
    char const * old_xdg_env = getenv( "XDG_CONFIG_HOME" );
    string saved_xdg;
    string * val;

    init( &saved_xdg );
    if ( old_xdg_env )
    {
        push_fmt( &saved_xdg, old_xdg_env );
    }

    mkdir( xdg_dir, 0700 );
    mkdir( xdg_sub, 0700 );
    write_text_file( xdg_ini, "[origin]\nsource = xdg\n" );
    setenv( "XDG_CONFIG_HOME", xdg_dir, 1 );

    /* 3) Fallback to global_config_file() via XDG_CONFIG_HOME */
    remove( cwd_ini );
    ctx_init();
    load_ini();
    val = get_setting( "origin", "source" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "xdg", first( val ) );
    ctx_cleanup();

    /* 2) Current directory crosscc.ini overrides global_config_file() */
    write_text_file( cwd_ini, "[origin]\nsource = cwd\n" );
    ctx_init();
    load_ini();
    val = get_setting( "origin", "source" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "cwd", first( val ) );
    ctx_cleanup();

    /* 1) Command-line g_ctx.config overrides both crosscc.ini and global_config_file() */
    write_text_file( cli_ini, "[origin]\nsource = cli\n" );
    ctx_init();
    push_fmt( &g_ctx.config, cli_ini );
    load_ini();
    val = get_setting( "origin", "source" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "cli", first( val ) );
    ctx_cleanup();

    /* Cleanup */
    remove( cli_ini );
    remove( cwd_ini );
    remove( xdg_ini );
    rmdir( xdg_sub );
    rmdir( xdg_dir );

    if ( old_xdg_env )
    {
        setenv( "XDG_CONFIG_HOME", first( &saved_xdg ), 1 );
    }
    else
    {
        unsetenv( "XDG_CONFIG_HOME" );
    }
    cleanup( &saved_xdg );

    PASS();
}

TEST test_load_target_ini_search_paths( void )
{
    char const * dir1 = "test_inidir1_tmp";
    char const * dir2 = "test_inidir2_tmp";
    char const * f1 = "test_inidir1_tmp/shared.ini";
    char const * f2 = "test_inidir2_tmp/shared.ini";
    char const * f3 = "test_inidir2_tmp/second_only.ini";
    string k, v;
    string * val;

    mkdir( dir1, 0700 );
    mkdir( dir2, 0700 );
    write_text_file( f1, "[target]\nfrom = dir1\n" );
    write_text_file( f2, "[target]\nfrom = dir2\n" );
    write_text_file( f3, "[target]\nfrom = dir2_only\n" );

    /* Target present in both dir1 and dir2: dir1 wins */
    ctx_init();
    init( &k );
    init( &v );
    push_fmt( &k, "settings.inifiles" );
    push_fmt( &v, "test_inidir1_tmp;test_inidir2_tmp/" );
    insert( &g_ctx.settings, k, v );
    push_fmt( &g_ctx.target, "shared" );

    load_target_ini();
    val = get_setting( "target", "from" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "dir1", first( val ) );
    ctx_cleanup();

    /* Target present only in second directory (with trailing slash) */
    ctx_init();
    init( &k );
    init( &v );
    push_fmt( &k, "settings.inifiles" );
    push_fmt( &v, "test_inidir1_tmp;test_inidir2_tmp/" );
    insert( &g_ctx.settings, k, v );
    push_fmt( &g_ctx.target, "second_only" );

    load_target_ini();
    val = get_setting( "target", "from" );
    ASSERT( val != NULL );
    ASSERT_STR_EQ( "dir2_only", first( val ) );
    ctx_cleanup();

    remove( f1 );
    remove( f2 );
    remove( f3 );
    rmdir( dir1 );
    rmdir( dir2 );

    PASS();
}

TEST test_list_targets( void )
{
    char const * dir1 = "test_list1_tmp";
    char const * dir2 = "test_list2_tmp";
    char const * t1 = "test_list1_tmp/alpha.ini";
    char const * t1_dup = "test_list2_tmp/alpha.ini";
    char const * t2 = "test_list2_tmp/beta.ini";
    char const * cfg_ini = "test_list1_tmp/config.ini";
    char const * crosscc_ini = "test_list2_tmp/crosscc.ini";
    char const * dot_ini = "test_list1_tmp/.ini";
    char const * non_ini = "test_list1_tmp/readme.txt";
    char const * sub_dir = "test_list1_tmp/subdir.ini";
    char const * out_file = "test_list_out_tmp.txt";
    string k, v;
    int saved_stdout;
    FILE * cap_fh;
    char buf[ 256 ];
    size_t nread;
    char * alpha_pos;

    mkdir( dir1, 0700 );
    mkdir( dir2, 0700 );
    mkdir( sub_dir, 0700 );
    write_text_file( t1, "[settings]\nname = alpha\n" );
    write_text_file( t1_dup, "[settings]\nname = alpha\n" );
    write_text_file( t2, "[settings]\nname = beta\n" );
    write_text_file( cfg_ini, "[settings]\n" );
    write_text_file( crosscc_ini, "[settings]\n" );
    write_text_file( dot_ini, "[settings]\n" );
    write_text_file( non_ini, "ignore me\n" );

    ctx_init();
    init( &k );
    init( &v );
    push_fmt( &k, "settings.inifiles" );
    push_fmt( &v, "test_list1_tmp;test_list2_tmp/" );
    insert( &g_ctx.settings, k, v );

    /* Redirect stdout to capture list_targets() output */
    fflush( stdout );
    saved_stdout = dup( STDOUT_FILENO );
    ASSERT( saved_stdout >= 0 );
    cap_fh = fopen( out_file, "w" );
    ASSERT( cap_fh != NULL );
    dup2( fileno( cap_fh ), STDOUT_FILENO );
    fclose( cap_fh );

    list_targets();

    fflush( stdout );
    dup2( saved_stdout, STDOUT_FILENO );
    close( saved_stdout );

    cap_fh = fopen( out_file, "r" );
    ASSERT( cap_fh != NULL );
    nread = fread( buf, 1, sizeof( buf ) - 1, cap_fh );
    buf[ nread ] = '\0';
    fclose( cap_fh );

    alpha_pos = strstr( buf, "alpha\n" );
    ASSERT( alpha_pos != NULL );
    ASSERT( strstr( alpha_pos + 1, "alpha\n" ) == NULL );
    ASSERT( strstr( buf, "beta\n" ) != NULL );
    ASSERT( strstr( buf, "config" ) == NULL );
    ASSERT( strstr( buf, "crosscc" ) == NULL );
    ASSERT( strstr( buf, "subdir" ) == NULL );
    ASSERT( strstr( buf, "readme" ) == NULL );

    /* Verbose mode: includes resolved path to first matching .ini file */
    g_ctx.verbose = 1;
    fflush( stdout );
    saved_stdout = dup( STDOUT_FILENO );
    ASSERT( saved_stdout >= 0 );
    cap_fh = fopen( out_file, "w" );
    ASSERT( cap_fh != NULL );
    dup2( fileno( cap_fh ), STDOUT_FILENO );
    fclose( cap_fh );

    list_targets();

    fflush( stdout );
    dup2( saved_stdout, STDOUT_FILENO );
    close( saved_stdout );

    cap_fh = fopen( out_file, "r" );
    ASSERT( cap_fh != NULL );
    nread = fread( buf, 1, sizeof( buf ) - 1, cap_fh );
    buf[ nread ] = '\0';
    fclose( cap_fh );

    ASSERT( strstr( buf, "alpha (test_list1_tmp/alpha.ini)\n" ) != NULL );
    ASSERT( strstr( buf, "test_list2_tmp/alpha.ini" ) == NULL );
    ASSERT( strstr( buf, "beta (test_list2_tmp/beta.ini)\n" ) != NULL );

    ctx_cleanup();

    remove( out_file );
    remove( t1 );
    remove( t1_dup );
    remove( t2 );
    remove( cfg_ini );
    remove( crosscc_ini );
    remove( dot_ini );
    remove( non_ini );
    rmdir( sub_dir );
    rmdir( dir1 );
    rmdir( dir2 );

    PASS();
}

static int run_resolve_mode_expect_exit( char const * mode )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        ctx_init();
        push_fmt( &g_ctx.config, "ini/config.ini" );
        load_ini();

        if ( mode )
        {
            push_fmt( &g_ctx.mode, mode );
        }

        resolve_mode();
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

TEST test_resolve_mode( void )
{
    ctx_init();
    push_fmt( &g_ctx.config, "ini/config.ini" );
    load_ini();

    push_fmt( &g_ctx.mode, "cc" );
    resolve_mode();
    ASSERT_STR_EQ( "compiler-c", first( &g_ctx.mode ) );

    clear( &g_ctx.mode );
    push_fmt( &g_ctx.mode, "ld" );
    resolve_mode();
    ASSERT_STR_EQ( "linker", first( &g_ctx.mode ) );

    ctx_cleanup();

    ASSERT_EQ( EXIT_FAILURE, run_resolve_mode_expect_exit( "bogus" ) );
    ASSERT_EQ( EXIT_FAILURE, run_resolve_mode_expect_exit( NULL ) );

    PASS();
}

SUITE( config_suite )
{
    RUN_TEST( test_load_repo_ini_and_target );
    RUN_TEST( test_ini_syntax_features );
    RUN_TEST( test_load_ini_precedence );
    RUN_TEST( test_load_target_ini_search_paths );
    RUN_TEST( test_list_targets );
    RUN_TEST( test_resolve_mode );
}
