#define _POSIX_C_SOURCE 200809L
#include "support/greatest.h"
#include "context.h"
#include "sysroot.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Expected test scenarios for sysroot provisioning.
 * Note: download/unpack/setup execution tests should use g_ctx.dryrun or
 * mock/stub shell commands (e.g. local scripts/files) to avoid network I/O.
 */

static void set_setting( char const * key_str, char const * val_str )
{
    string k, v;
    init( &k );
    init( &v );
    push_fmt( &k, key_str );
    push_fmt( &v, val_str );
    insert( &g_ctx.settings, k, v );
}

TEST test_sysroot_exists( void )
{
    char const * base_dir = "test_sysroot_exists_tmp";
    char const * sub_dir = "test_sysroot_exists_tmp/sub";

    rmdir( sub_dir );
    rmdir( base_dir );

    ctx_init();

    /* Unset sysroot.dir -> false */
    ASSERT_EQ( false, sysroot_exists() );

    /* Non-existent expanded path -> false */
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    ASSERT_EQ( false, sysroot_exists() );

    /* Existing readable directory -> true */
    ASSERT_EQ( 0, mkdir( base_dir, 0755 ) );
    ASSERT_EQ( 0, mkdir( sub_dir, 0755 ) );
    ASSERT_EQ( true, sysroot_exists() );

    ctx_cleanup();

    rmdir( sub_dir );
    rmdir( base_dir );

    PASS();
}

static int run_cache_file_expect_exit( void )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        sysroot_cache_file();
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

TEST test_sysroot_cache_file( void )
{
    char const * dir1 = "test_cache_dir1_tmp";
    char const * dir2 = "test_cache_dir2_tmp";
    char const * hit_file = "test_cache_dir2_tmp/archive.tar.xz";
    char const * local_file = "test_cache_dir2_tmp/custom.tgz";
    sysroot_archive_t res;
    FILE * fh;

    remove( hit_file );
    remove( local_file );
    rmdir( dir1 );
    rmdir( dir2 );

    ASSERT_EQ( 0, mkdir( dir1, 0755 ) );
    ASSERT_EQ( 0, mkdir( dir2, 0755 ) );

    ctx_init();

    /* Missing sysroot.url and sysroot.local -> empty path and basename */
    set_setting( "settings.cache", "test_cache_dir1_tmp;test_cache_dir2_tmp" );
    res = sysroot_cache_file();
    ASSERT_EQ( (size_t)0, size( &res.path ) );
    ASSERT_EQ( (size_t)0, size( &res.basename ) );
    cleanup( &res.path );
    cleanup( &res.basename );

    /* Miss across cache dirs -> first dir in .path, URL basename in .basename */
    set_setting( "sysroot.url", "https://example.com/downloads/archive.tar.xz" );
    res = sysroot_cache_file();
    ASSERT_STR_EQ( dir1, first( &res.path ) );
    ASSERT_STR_EQ( "archive.tar.xz", first( &res.basename ) );
    cleanup( &res.path );
    cleanup( &res.basename );

    /* Hit in second cache dir -> full path in .path, empty .basename */
    fh = fopen( hit_file, "wb" );
    ASSERT( fh != NULL );
    fclose( fh );

    res = sysroot_cache_file();
    ASSERT_STR_EQ( hit_file, first( &res.path ) );
    ASSERT_EQ( (size_t)0, size( &res.basename ) );
    cleanup( &res.path );
    cleanup( &res.basename );

    /* sysroot.local overrides URL basename on miss and hit */
    set_setting( "sysroot.local", "custom.tgz" );
    res = sysroot_cache_file();
    ASSERT_STR_EQ( dir1, first( &res.path ) );
    ASSERT_STR_EQ( "custom.tgz", first( &res.basename ) );
    cleanup( &res.path );
    cleanup( &res.basename );

    fh = fopen( local_file, "wb" );
    ASSERT( fh != NULL );
    fclose( fh );

    res = sysroot_cache_file();
    ASSERT_STR_EQ( local_file, first( &res.path ) );
    ASSERT_EQ( (size_t)0, size( &res.basename ) );
    cleanup( &res.path );
    cleanup( &res.basename );

    /* sysroot.local with path separator exits with EXIT_FAILURE */
    set_setting( "sysroot.local", "sub/custom.tgz" );
    ASSERT_EQ( EXIT_FAILURE, run_cache_file_expect_exit() );

    ctx_cleanup();

    remove( hit_file );
    remove( local_file );
    rmdir( dir1 );
    rmdir( dir2 );

    PASS();
}

static int run_verify_expect_exit( char const * archive, char const * expected )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        sysroot_verify_checksum( archive, expected );
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

TEST test_sysroot_verify_checksum( void )
{
    char const * tmp_file = "test_checksum_tmp.bin";
    /* NIST SHA-256 test vector for "abc" */
    char const * abc_lower = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    char const * abc_upper = "BA7816BF8F01CFEA414140DE5DAE2223B00361A396177A9CB410FF61F20015AD";
    char const * abc_wrong = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ae";
    FILE * fh;

    /* Empty or NULL archive returns false */
    ASSERT_EQ( false, sysroot_verify_checksum( NULL, abc_lower ) );
    ASSERT_EQ( false, sysroot_verify_checksum( "", abc_lower ) );

    /* Empty or NULL expected checksum skips verification and returns true */
    ASSERT_EQ( true, sysroot_verify_checksum( tmp_file, NULL ) );
    ASSERT_EQ( true, sysroot_verify_checksum( tmp_file, "" ) );

    /* Create temp file with "abc" */
    fh = fopen( tmp_file, "wb" );
    ASSERT( fh != NULL );
    ASSERT_EQ( 3, fwrite( "abc", 1, 3, fh ) );
    fclose( fh );

    /* Matching lowercase and uppercase hex digests */
    ASSERT_EQ( true, sysroot_verify_checksum( tmp_file, abc_lower ) );
    ASSERT_EQ( true, sysroot_verify_checksum( tmp_file, abc_upper ) );

    /* Mismatched digest returns false */
    ASSERT_EQ( false, sysroot_verify_checksum( tmp_file, abc_wrong ) );

    remove( tmp_file );

    /* Missing file exits with EXIT_FAILURE */
    ASSERT_EQ( EXIT_FAILURE, run_verify_expect_exit( "nonexistent_checksum_file.bin", abc_lower ) );

    PASS();
}

static int run_provision_expect_exit( void )
{
    pid_t pid = fork();

    if ( pid == 0 )
    {
        provision_sysroot();
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

TEST test_provision_sysroot_already_exists( void )
{
    char const * existing_dir = "test_provision_exists_tmp";

    rmdir( existing_dir );
    ASSERT_EQ( 0, mkdir( existing_dir, 0755 ) );

    ctx_init();
    set_setting( "sysroot.dir", existing_dir );

    /* Returns early without needing sysroot.url or settings.cache */
    provision_sysroot();

    ctx_cleanup();
    rmdir( existing_dir );

    PASS();
}

TEST test_provision_sysroot_dryrun( void )
{
    char const * cache_dir = "test_provision_dryrun_cache_tmp";
    char const * archive_file = "test_provision_dryrun_cache_tmp/sysroot.tar.xz";
    char const * base_dir = "test_provision_dryrun_base_tmp";
    FILE * fh;

    remove( archive_file );
    rmdir( cache_dir );
    rmdir( base_dir );

    ASSERT_EQ( 0, mkdir( cache_dir, 0755 ) );
    fh = fopen( archive_file, "wb" );
    ASSERT( fh != NULL );
    ASSERT_EQ( 3, fwrite( "abc", 1, 3, fh ) );
    fclose( fh );

    /* Dry run with cached archive */
    ctx_init();
    g_ctx.dryrun = true;
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.checksum", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    set_setting( "sysroot.unpack", "false {ARGS:1}" );
    set_setting( "sysroot.setup", "false" );

    provision_sysroot();

    /* dryrun must not create sysroot.base or execute unpack/setup */
    ASSERT_EQ( false, dir_readable( base_dir ) );

    ctx_cleanup();

    /* Dry run with uncached archive */
    remove( archive_file );

    ctx_init();
    g_ctx.dryrun = true;
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.download", "false {ARGS:1} {ARGS:2}" );
    set_setting( "sysroot.checksum", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    set_setting( "sysroot.unpack", "false {ARGS:1}" );
    set_setting( "sysroot.setup", "false" );

    provision_sysroot();

    ASSERT_EQ( false, file_readable( archive_file ) );
    ASSERT_EQ( false, dir_readable( base_dir ) );

    ctx_cleanup();

    rmdir( cache_dir );
    rmdir( base_dir );

    PASS();
}

TEST test_provision_sysroot_unpack_and_setup( void )
{
    char const * cache_dir = "test_provision_run_cache_tmp";
    char const * archive_file = "test_provision_run_cache_tmp/sysroot.tar.xz";
    char const * base_dir = "test_provision_run_base_tmp";
    char const * sub_dir = "test_provision_run_base_tmp/sub";
    char const * setup_marker = "test_provision_run_base_tmp/sub/setup_done";
    FILE * fh;

    remove( setup_marker );
    rmdir( sub_dir );
    rmdir( base_dir );
    remove( archive_file );
    rmdir( cache_dir );

    ASSERT_EQ( 0, mkdir( cache_dir, 0755 ) );
    fh = fopen( archive_file, "wb" );
    ASSERT( fh != NULL );
    ASSERT_EQ( 3, fwrite( "abc", 1, 3, fh ) );
    fclose( fh );

    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    set_setting( "sysroot.unpack", "test -f {ARGS:1} && mkdir -p {sysroot.dir}" );
    set_setting( "sysroot.setup", "touch {sysroot.dir}/setup_done" );

    provision_sysroot();

    ASSERT_EQ( true, sysroot_exists() );
    ASSERT_EQ( true, file_readable( setup_marker ) );

    ctx_cleanup();

    remove( setup_marker );
    rmdir( sub_dir );
    rmdir( base_dir );
    remove( archive_file );
    rmdir( cache_dir );

    PASS();
}

TEST test_provision_sysroot_errors( void )
{
    char const * cache_dir = "test_provision_err_cache_tmp";
    char const * archive_file = "test_provision_err_cache_tmp/sysroot.tar.xz";
    char const * base_dir = "test_provision_err_base_tmp";
    char const * sub_dir = "test_provision_err_base_tmp/sub";
    FILE * fh;

    rmdir( sub_dir );
    rmdir( base_dir );
    remove( archive_file );
    rmdir( cache_dir );

    /* Missing sysroot.url and settings.cache -> EXIT_FAILURE */
    ctx_init();
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Cached archive with bad checksum -> EXIT_FAILURE */
    ASSERT_EQ( 0, mkdir( cache_dir, 0755 ) );
    fh = fopen( archive_file, "wb" );
    ASSERT( fh != NULL );
    ASSERT_EQ( 3, fwrite( "abc", 1, 3, fh ) );
    fclose( fh );

    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.checksum", "0000000000000000000000000000000000000000000000000000000000000000" );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Missing sysroot.base -> EXIT_FAILURE */
    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.dir", sub_dir );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Missing sysroot.dir -> EXIT_FAILURE */
    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.base", base_dir );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Missing sysroot.unpack -> EXIT_FAILURE */
    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Failing sysroot.unpack -> EXIT_FAILURE */
    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", "{sysroot.base}/sub" );
    set_setting( "sysroot.unpack", "false" );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    /* Failing sysroot.setup -> EXIT_FAILURE */
    ctx_init();
    set_setting( "settings.cache", cache_dir );
    set_setting( "sysroot.url", "https://example.com/sysroot.tar.xz" );
    set_setting( "sysroot.base", base_dir );
    set_setting( "sysroot.dir", sub_dir );
    set_setting( "sysroot.unpack", "mkdir -p {sysroot.dir}" );
    set_setting( "sysroot.setup", "false" );
    ASSERT_EQ( EXIT_FAILURE, run_provision_expect_exit() );
    ctx_cleanup();

    rmdir( sub_dir );
    rmdir( base_dir );
    remove( archive_file );
    rmdir( cache_dir );

    PASS();
}

SUITE( sysroot_suite )
{
    RUN_TEST( test_sysroot_exists );
    RUN_TEST( test_sysroot_cache_file );
    RUN_TEST( test_sysroot_verify_checksum );
    RUN_TEST( test_provision_sysroot_already_exists );
    RUN_TEST( test_provision_sysroot_dryrun );
    RUN_TEST( test_provision_sysroot_unpack_and_setup );
    RUN_TEST( test_provision_sysroot_errors );
}
