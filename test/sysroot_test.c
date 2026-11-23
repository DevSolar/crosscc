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

TEST test_sysroot_cache_file( void )
{
    /* Scenarios:
     * - Resolves archive basename from sysroot.url into first settings.cache dir.
     * - Returns existing cached file from a subsequent settings.cache dir if present.
     * - Falls back to XDG_CACHE_HOME / HOME when settings.cache is unset.
     */
    SKIPm( "TODO: implement sysroot_cache_file() tests" );
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

TEST test_provision_sysroot_already_exists( void )
{
    /* Scenarios:
     * - When sysroot.dir already exists, returns early without downloading/unpacking.
     */
    SKIPm( "TODO: implement provision_sysroot() existing-dir test" );
}

TEST test_provision_sysroot_dryrun( void )
{
    /* Scenarios:
     * - With missing sysroot.dir and g_ctx.dryrun = true, logs download/unpack/setup
     *   actions without creating files or executing external commands.
     */
    SKIPm( "TODO: implement provision_sysroot() dryrun test" );
}

SUITE( sysroot_suite )
{
    RUN_TEST( test_sysroot_exists );
    RUN_TEST( test_sysroot_cache_file );
    RUN_TEST( test_sysroot_verify_checksum );
    RUN_TEST( test_provision_sysroot_already_exists );
    RUN_TEST( test_provision_sysroot_dryrun );
}
