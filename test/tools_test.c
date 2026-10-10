#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "support/greatest.h"
#include "execute.h"
#include "tools.h"
#include "utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool write_test_file( char const * path, char const * content )
{
    FILE * f = fopen( path, "wb" );
    size_t len;

    if ( ! f )
    {
        return false;
    }

    if ( content != NULL && *content != '\0' )
    {
        len = strlen( content );
        if ( fwrite( content, 1, len, f ) != len )
        {
            fclose( f );
            return false;
        }
    }

    fclose( f );
    return true;
}

static bool read_test_file( char const * path, char * buf, size_t max_len )
{
    FILE * f = fopen( path, "rb" );
    size_t n;

    if ( ! f )
    {
        return false;
    }

    n = fread( buf, 1, max_len - 1, f );
    buf[n] = '\0';
    fclose( f );
    return true;
}

static bool create_test_tar( char const * tar_path, char const * src_dir, char const * file_name )
{
    vec( string ) cmd;
    int rc;

    init( &cmd );
    push( &cmd, "tar" );
    push( &cmd, "-cf" );
    push( &cmd, tar_path );
    push( &cmd, "-C" );
    push( &cmd, src_dir );
    push( &cmd, file_name );
    rc = execute( &cmd );
    cleanup( &cmd );

    return rc == 0;
}

static void cleanup_test_dir( void * udata )
{
    vec( string ) opts;
    (void)udata;

    init( &opts );
    push( &opts, "--all" );
    push( &opts, "--force" );
    push( &opts, "tmp_tools_test" );
    rm( &opts );
    cleanup( &opts );
}

static void setup_test_dir( void * udata )
{
    vec( string ) opts;
    (void)udata;

    cleanup_test_dir( NULL );
    init( &opts );
    push( &opts, "tmp_tools_test" );
    makedir( &opts );
    cleanup( &opts );
}

TEST test_tools_echo( void )
{
    vec( string ) opts;
    init( &opts );

    /* Empty options -> true */
    ASSERT_EQ( true, echo( &opts ) );

    /* With multiple options -> true */
    push( &opts, "hello" );
    push( &opts, "world" );
    ASSERT_EQ( true, echo( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_set_env( void )
{
    vec( string ) opts;
    init( &opts );

    /* Wrong argument count -> false */
    ASSERT_EQ( false, set_env( &opts ) );

    push( &opts, "CROSSCC_TEST_VAR" );
    ASSERT_EQ( false, set_env( &opts ) );

    push( &opts, "val1" );
    push( &opts, "extra" );
    ASSERT_EQ( false, set_env( &opts ) );

    /* Valid 2 arguments -> true */
    erase( &opts, 2 );
    ASSERT_EQ( true, set_env( &opts ) );
    ASSERT_STR_EQ( "val1", getenv( "CROSSCC_TEST_VAR" ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_unset_env( void )
{
    vec( string ) set_opts;
    vec( string ) unset_opts;
    init( &set_opts );
    init( &unset_opts );

    /* First set the variable */
    push( &set_opts, "CROSSCC_TEST_UNSET" );
    push( &set_opts, "foobar" );
    ASSERT_EQ( true, set_env( &set_opts ) );
    ASSERT_STR_EQ( "foobar", getenv( "CROSSCC_TEST_UNSET" ) );

    /* 0 arguments -> false */
    ASSERT_EQ( false, unset_env( &unset_opts ) );

    /* Unset the variable -> true */
    push( &unset_opts, "CROSSCC_TEST_UNSET" );
    ASSERT_EQ( true, unset_env( &unset_opts ) );
    ASSERT_EQ( NULL, getenv( "CROSSCC_TEST_UNSET" ) );

    cleanup( &set_opts );
    cleanup( &unset_opts );
    PASS();
}

TEST test_tools_makedir( void )
{
    vec( string ) opts;
    init( &opts );

    /* 0 arguments -> false */
    ASSERT_EQ( false, makedir( &opts ) );

    /* Create nested directory -> true */
    push( &opts, "tmp_tools_test/sub1/sub2/sub3" );
    ASSERT_EQ( true, makedir( &opts ) );
    ASSERT_EQ( true, dir_readable( "tmp_tools_test/sub1/sub2/sub3" ) );

    /* Creating already existing directory -> true */
    ASSERT_EQ( true, makedir( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_touch( void )
{
    vec( string ) opts;
    init( &opts );

    /* 0 arguments -> false */
    ASSERT_EQ( false, touch( &opts ) );

    /* Touching non-existent file without --create -> false */
    push( &opts, "tmp_tools_test/new_file.txt" );
    ASSERT_EQ( false, touch( &opts ) );
    ASSERT_EQ( false, file_readable( "tmp_tools_test/new_file.txt" ) );

    /* Touching with --create -> true */
    push( &opts, "--create" );
    ASSERT_EQ( true, touch( &opts ) );
    ASSERT_EQ( true, file_readable( "tmp_tools_test/new_file.txt" ) );

    /* Touching existing file without --create -> true */
    cleanup( &opts );
    init( &opts );
    push( &opts, "tmp_tools_test/new_file.txt" );
    ASSERT_EQ( true, touch( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_copy( void )
{
    vec( string ) opts;
    char read_buf[64];
    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, copy( &opts ) );
    push( &opts, "tmp_tools_test/copy_src.txt" );
    ASSERT_EQ( false, copy( &opts ) );

    /* Non-existent source -> false */
    push( &opts, "tmp_tools_test/copy_dst.txt" );
    ASSERT_EQ( false, copy( &opts ) );

    /* Create source file and copy -> true */
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/copy_src.txt", "copy test content" ) );
    ASSERT_EQ( true, copy( &opts ) );
    ASSERT_EQ( true, file_readable( "tmp_tools_test/copy_dst.txt" ) );
    ASSERT_EQ( true, read_test_file( "tmp_tools_test/copy_dst.txt", read_buf, sizeof( read_buf ) ) );
    ASSERT_STR_EQ( "copy test content", read_buf );

    /* Test --if-newer flag when destination is up to date -> true */
    push( &opts, "--if-newer" );
    ASSERT_EQ( true, copy( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_copydir( void )
{
    vec( string ) mk_opts;
    vec( string ) opts;
    char read_buf[64];
    init( &mk_opts );
    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, copydir( &opts ) );
    push( &opts, "tmp_tools_test/tree_src" );
    ASSERT_EQ( false, copydir( &opts ) );

    /* Non-existent source -> false */
    push( &opts, "tmp_tools_test/tree_dst" );
    ASSERT_EQ( false, copydir( &opts ) );

    /* Setup source directory tree with nested files */
    push( &mk_opts, "tmp_tools_test/tree_src/nested" );
    ASSERT_EQ( true, makedir( &mk_opts ) );
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/tree_src/f1.txt", "root file" ) );
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/tree_src/nested/f2.txt", "nested file" ) );

    /* Copy directory tree -> true */
    ASSERT_EQ( true, copydir( &opts ) );
    ASSERT_EQ( true, dir_readable( "tmp_tools_test/tree_dst" ) );
    ASSERT_EQ( true, dir_readable( "tmp_tools_test/tree_dst/nested" ) );
    ASSERT_EQ( true, file_readable( "tmp_tools_test/tree_dst/f1.txt" ) );
    ASSERT_EQ( true, file_readable( "tmp_tools_test/tree_dst/nested/f2.txt" ) );

    ASSERT_EQ( true, read_test_file( "tmp_tools_test/tree_dst/nested/f2.txt", read_buf, sizeof( read_buf ) ) );
    ASSERT_STR_EQ( "nested file", read_buf );

    cleanup( &mk_opts );
    cleanup( &opts );
    PASS();
}

TEST test_tools_concat( void )
{
    vec( string ) opts;
    init( &opts );

    /* 0 arguments -> false */
    ASSERT_EQ( false, concat( &opts ) );

    /* Non-existent file -> false */
    push( &opts, "tmp_tools_test/non_existent.txt" );
    ASSERT_EQ( false, concat( &opts ) );

    /* Valid files -> true */
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/c1.txt", "line 1\n" ) );
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/c2.txt", "line 2\n" ) );

    cleanup( &opts );
    init( &opts );
    push( &opts, "tmp_tools_test/c1.txt" );
    push( &opts, "tmp_tools_test/c2.txt" );
    ASSERT_EQ( true, concat( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_checksum( void )
{
    vec( string ) opts;
    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, checksum( &opts ) );

    push( &opts, "tmp_tools_test/sha.txt" );
    push( &opts, "tmp_tools_test/extra.txt" );
    ASSERT_EQ( false, checksum( &opts ) );

    /* Non-existent file -> false */
    erase( &opts, 1 );
    ASSERT_EQ( false, checksum( &opts ) );

    /* Valid file -> true */
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/sha.txt", "hello sha256" ) );
    ASSERT_EQ( true, checksum( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_link( void )
{
    vec( string ) opts;
    char read_buf[64];

    remove( "tmp_test_link.txt" );
    remove( "tmp_test_target.txt" );

    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, link( &opts ) );
    push( &opts, "tmp_test_link.txt" );
    ASSERT_EQ( false, link( &opts ) );

    /* Valid link (target file exists) */
    ASSERT_EQ( true, write_test_file( "tmp_test_target.txt", "link target content" ) );

    push( &opts, "tmp_test_target.txt" );
    ASSERT_EQ( true, link( &opts ) );
    ASSERT_EQ( true, file_readable( "tmp_test_link.txt" ) );
    ASSERT_EQ( true, read_test_file( "tmp_test_link.txt", read_buf, sizeof( read_buf ) ) );
    ASSERT_STR_EQ( "link target content", read_buf );

    remove( "tmp_test_link.txt" );
    remove( "tmp_test_target.txt" );

    cleanup( &opts );
    PASS();
}

TEST test_tools_sleep( void )
{
    vec( string ) opts;
    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, sleep( &opts ) );

    /* Invalid duration string -> false */
    push( &opts, "not_a_number" );
    ASSERT_EQ( false, sleep( &opts ) );

    /* Negative duration -> false */
    cleanup( &opts );
    init( &opts );
    push( &opts, "-1.5" );
    ASSERT_EQ( false, sleep( &opts ) );

    /* Valid small duration -> true */
    cleanup( &opts );
    init( &opts );
    push( &opts, "0.005" );
    ASSERT_EQ( true, sleep( &opts ) );

    cleanup( &opts );
    PASS();
}

TEST test_tools_rm( void )
{
    vec( string ) opts;
    vec( string ) mk_opts;
    init( &opts );
    init( &mk_opts );

    /* 0 arguments -> false */
    ASSERT_EQ( false, rm( &opts ) );

    /* Non-existent file without --force -> false */
    push( &opts, "tmp_tools_test/no_such_file.txt" );
    ASSERT_EQ( false, rm( &opts ) );

    /* Non-existent file with --force -> true */
    push( &opts, "--force" );
    ASSERT_EQ( true, rm( &opts ) );

    /* Remove existing file -> true */
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/rm_file.txt", "delete me" ) );
    ASSERT_EQ( true, file_readable( "tmp_tools_test/rm_file.txt" ) );

    cleanup( &opts );
    init( &opts );
    push( &opts, "tmp_tools_test/rm_file.txt" );
    ASSERT_EQ( true, rm( &opts ) );
    ASSERT_EQ( false, file_readable( "tmp_tools_test/rm_file.txt" ) );

    /* Remove directory without --all -> false */
    push( &mk_opts, "tmp_tools_test/dir_rm/sub" );
    ASSERT_EQ( true, makedir( &mk_opts ) );
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/dir_rm/sub/f.txt", "nested" ) );

    cleanup( &opts );
    init( &opts );
    push( &opts, "tmp_tools_test/dir_rm" );
    ASSERT_EQ( false, rm( &opts ) );
    ASSERT_EQ( true, dir_readable( "tmp_tools_test/dir_rm" ) );

    /* Remove directory with --all -> true */
    push( &opts, "--all" );
    ASSERT_EQ( true, rm( &opts ) );
    ASSERT_EQ( false, dir_readable( "tmp_tools_test/dir_rm" ) );

    cleanup( &mk_opts );
    cleanup( &opts );
    PASS();
}

TEST test_tools_unpack( void )
{
    vec( string ) mk_opts;
    vec( string ) opts;
    char read_buf[64];
    bool tar_ok;
    init( &mk_opts );
    init( &opts );

    /* Invalid argument count -> false */
    ASSERT_EQ( false, unpack( &opts ) );

    push( &opts, "a" );
    push( &opts, "b" );
    push( &opts, "c" );
    ASSERT_EQ( false, unpack( &opts ) );

    /* Non-existent archive -> false */
    cleanup( &opts );
    init( &opts );
    push( &opts, "tmp_tools_test/no_such_archive.tar" );
    ASSERT_EQ( false, unpack( &opts ) );

    /* Create sample archive and extract it */
    push( &mk_opts, "tmp_tools_test/tar_in" );
    push( &mk_opts, "tmp_tools_test/tar_out" );
    ASSERT_EQ( true, makedir( &mk_opts ) );
    ASSERT_EQ( true, write_test_file( "tmp_tools_test/tar_in/archived.txt", "tar payload" ) );

    tar_ok = create_test_tar( "tmp_tools_test/test.tar", "tmp_tools_test/tar_in", "archived.txt" );
    if ( tar_ok )
    {
        cleanup( &opts );
        init( &opts );
        push( &opts, "tmp_tools_test/test.tar" );
        push( &opts, "tmp_tools_test/tar_out" );
        ASSERT_EQ( true, unpack( &opts ) );
        ASSERT_EQ( true, file_readable( "tmp_tools_test/tar_out/archived.txt" ) );
        ASSERT_EQ( true, read_test_file( "tmp_tools_test/tar_out/archived.txt", read_buf, sizeof( read_buf ) ) );
        ASSERT_STR_EQ( "tar payload", read_buf );
    }

    cleanup( &mk_opts );
    cleanup( &opts );
    PASS();
}

SUITE( tools_suite )
{
    SET_SETUP( setup_test_dir, NULL );
    SET_TEARDOWN( cleanup_test_dir, NULL );

    RUN_TEST( test_tools_echo );
    RUN_TEST( test_tools_set_env );
    RUN_TEST( test_tools_unset_env );
    RUN_TEST( test_tools_makedir );
    RUN_TEST( test_tools_touch );
    RUN_TEST( test_tools_copy );
    RUN_TEST( test_tools_copydir );
    RUN_TEST( test_tools_concat );
    RUN_TEST( test_tools_checksum );
    RUN_TEST( test_tools_link );
    RUN_TEST( test_tools_sleep );
    RUN_TEST( test_tools_rm );
    RUN_TEST( test_tools_unpack );
}
