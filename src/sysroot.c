#include "sysroot.h"

#include "context.h"
#include "utils.h"

#define LONESHA256_STATIC
#define LONESHA256_STDIO
#include "support/lonesha256.h"
#define MKDIR_P_STATIC
#define MKDIR_P_IMPLEMENTATION
#include "support/mkdir_p.h"
#include "support/solog.h"

bool sysroot_exists( void )
{
    string dir = get_setting_expanded( "sysroot", "dir" );
    bool exists = ( size( &dir ) > 0 ) && ( dir_readable( first( &dir ) ) );
    cleanup( &dir );
    return exists;
}

static bool sysroot_cache_file_cb( string * dir, void * udata )
{
    sysroot_archive_t * archive = (sysroot_archive_t *)udata;
    string filename;

    init( &filename );

    /* Add 'dir', conditionally with separator */
    if ( dir && size( dir ) > 0 )
    {
        push_fmt( &filename, first( dir ) );

        if ( *last( dir ) != '/' && *last( dir ) != '\\' )
        {
            push_fmt( &filename, "/" );
        }
    }

    /* Add 'basename' */
    push_fmt( &filename, first( &archive->basename ) );

    if ( file_readable( first( &filename ) ) )
    {
        cleanup( &archive->path );
        archive->path = filename;
        cleanup( &archive->basename );
        return false;
    }

    cleanup( &filename );

    if ( size( &archive->path ) == 0 && dir && size( dir ) > 0 )
    {
        push_fmt( &archive->path, first( dir ) );
    }

    return true;
}

sysroot_archive_t sysroot_cache_file( void )
{
    string url = get_setting_expanded( "sysroot", "url" );
    string local = get_setting_expanded( "sysroot", "local" );
    string cache_dirs = get_setting_expanded( "settings", "cache" );
    sysroot_archive_t archive;

    init( &archive.basename );
    init( &archive.path );

    if ( size( &local ) > 0 )
    {
        strspan_t local_span = { first( &local ), end( &local ) };

        if ( strrpbrk( &local_span, "/\\" ) != NULL )
        {
            SOLOG( ERR, "sysroot.local must not contain path separators: '%s'", first( &local ) );
            cleanup( &url );
            cleanup( &local );
            cleanup( &cache_dirs );
            exit( EXIT_FAILURE );
        }

        push_fmt( &archive.basename, first( &local ) );
    }
    else
    {
        strspan_t url_span = { first( &url ), end( &url ) };
        char const * sep = strrpbrk( &url_span, "/\\" );
        char const * basename = sep ? sep + 1 : first( &url );

        if ( basename && *basename )
        {
            push_fmt( &archive.basename, basename );
        }
    }

    if ( size( &archive.basename ) > 0 )
    {
        foreach( first( &cache_dirs ), ";", sysroot_cache_file_cb, &archive );
    }

    cleanup( &url );
    cleanup( &local );
    cleanup( &cache_dirs );
    return archive;
}

bool sysroot_verify_checksum( char const * archive, char const * expected )
{
    unsigned char sha256[32];
    char sha256_hex[65];
    int rc;

    (void)&lonesha256;

    if ( ! archive || ! *archive )
    {
        SOLOG( ERR, "Empty archive name for sysroot_verify_checksum()" );
        return false;
    }

    if ( ! expected || ! *expected )
    {
        SOLOG( WARN, "No checksum for \"%s\"", archive );
        return true;
    }

    if ( ( rc = lonesha256_file( sha256, archive ) ) )
    {
        SOLOG( ERR, "Error in lonesha256: %s", rc == 1 ? "Illegal Arguments" : ( rc == 2 ) ? "Could not open file" : "Read error" );
        exit( EXIT_FAILURE );
    }

    sprintf( sha256_hex,
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"
        "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        sha256[0], sha256[1], sha256[2], sha256[3],
        sha256[4], sha256[5], sha256[6], sha256[7],
        sha256[8], sha256[9], sha256[10], sha256[11],
        sha256[12], sha256[13], sha256[14], sha256[15],
        sha256[16], sha256[17], sha256[18], sha256[19],
        sha256[20], sha256[21], sha256[22], sha256[23],
        sha256[24], sha256[25], sha256[26], sha256[27],
        sha256[28], sha256[29], sha256[30], sha256[31] );

    sha256_hex[64] = '\0';
    return stricmp( sha256_hex, expected ) == 0;
}

static void sysroot_archive_cleanup( sysroot_archive_t * archive )
{
    cleanup( &archive->path );
    cleanup( &archive->basename );
}

static bool run_sysroot_command( char const * key, string * command )
{
    int rc;

    if ( g_ctx.dryrun )
    {
        SOLOG( INFO, "[dryrun] %s", first( command ) );
        return true;
    }

    rc = system( first( command ) );

    if ( rc != 0 )
    {
        SOLOG( ERR, "sysroot.%s command failed (exit %d): %s", key, rc, first( command ) );
        return false;
    }

    return true;
}

static void append_archive_basename( string * path, string * basename )
{
    if ( size( path ) > 0 && *last( path ) != '/' && *last( path ) != '\\' )
    {
        push_fmt( path, "/" );
    }

    push_fmt( path, first( basename ) );
}

void sysroot_archive_download( sysroot_archive_t * archive )
{
    string command;

    if ( ! archive )
    {
        return;
    }

    command = get_setting_expanded( "sysroot", "download", first( &archive->path ), first( &archive->basename ) );

    if ( size( &command ) == 0 )
    {
        SOLOG( ERR, "Empty sysroot.download command" );
        cleanup( &command );
        goto fail;
    }

    if ( g_ctx.dryrun )
    {
        SOLOG( INFO, "[dryrun] %s", first( &command ) );
        cleanup( &command );
        append_archive_basename( &archive->path, &archive->basename );
        return;
    }

    if ( ! run_sysroot_command( "download", &command ) )
    {
        SOLOG( ERR, "Download of sysroot archive '%s' failed", first( &archive->basename ) );
        cleanup( &command );
        goto fail;
    }

    cleanup( &command );

    sysroot_archive_cleanup( archive );
    *archive = sysroot_cache_file();

    if ( size( &archive->basename ) > 0 )
    {
        SOLOG( ERR, "Attempted download of sysroot archive '%s' but file does not exist", first( &archive->basename ) );
        goto fail;
    }

    return;

fail:
    sysroot_archive_cleanup( archive );
    exit( EXIT_FAILURE );
}

void provision_sysroot( void )
{
    sysroot_archive_t archive;
    string checksum_expected;
    string base;
    string dir;
    string command;
    bool skip_checksum;
    bool checksum_ok;

    if ( sysroot_exists() )
    {
        return;
    }

    init( &base );
    init( &dir );
    init( &command );

    archive = sysroot_cache_file();

    if ( size( &archive.path ) == 0 )
    {
        SOLOG( ERR, "Could not resolve sysroot archive path (check sysroot.url and settings.cache)" );
        goto fail;
    }

    skip_checksum = g_ctx.dryrun && ( size( &archive.basename ) > 0 );

    if ( size( &archive.basename ) > 0 )
    {
        sysroot_archive_download( &archive );
    }

    checksum_expected = get_setting_expanded( "sysroot", "checksum" );

    if ( skip_checksum )
    {
        if ( size( &checksum_expected ) > 0 )
        {
            SOLOG( INFO, "[dryrun] verify sha256 '%s' == %s", first( &archive.path ), first( &checksum_expected ) );
        }

        cleanup( &checksum_expected );
    }
    else
    {
        checksum_ok = sysroot_verify_checksum( first( &archive.path ), first( &checksum_expected ) );
        cleanup( &checksum_expected );

        if ( ! checksum_ok )
        {
            SOLOG( ERR, "Checksum error in '%s', sysroot provision denied", first( &archive.path ) );
            goto fail;
        }
    }

    base = get_setting_expanded( "sysroot", "base" );

    if ( size( &base ) == 0 )
    {
        SOLOG( ERR, "sysroot.base is not configured" );
        goto fail;
    }

    dir = get_setting_expanded( "sysroot", "dir" );

    if ( size( &dir ) == 0 )
    {
        SOLOG( ERR, "sysroot.dir is not configured" );
        goto fail;
    }

    {
        size_t base_len = size( &base );

        while ( base_len > 1 && ( first( &base )[base_len - 1] == '/' || first( &base )[base_len - 1] == '\\' ) )
        {
            --base_len;
        }

        if ( size( &dir ) < base_len ||
             strncmp( first( &dir ), first( &base ), base_len ) != 0 ||
             ( size( &dir ) > base_len && first( &dir )[base_len] != '/' && first( &dir )[base_len] != '\\' ) )
        {
            SOLOG( WARN, "sysroot.dir ('%s') is not under sysroot.base ('%s')", first( &dir ), first( &base ) );
        }
    }

    if ( g_ctx.dryrun )
    {
        if ( ! dir_readable( first( &base ) ) )
        {
            SOLOG( INFO, "[dryrun] mkdir -p '%s'", first( &base ) );
        }
    }
    else if ( mkdir_p( first( &base ), 0755 ) != 0 )
    {
        SOLOG( ERR, "Failed to create sysroot.base directory '%s'", first( &base ) );
        goto fail;
    }

    command = get_setting_expanded( "sysroot", "unpack", first( &archive.path ) );

    if ( size( &command ) == 0 )
    {
        SOLOG( ERR, "Empty sysroot.unpack command" );
        goto fail;
    }

    if ( ! run_sysroot_command( "unpack", &command ) )
    {
        goto fail;
    }

    cleanup( &command );
    init( &command );

    command = get_setting_expanded( "sysroot", "setup" );

    if ( size( &command ) > 0 && ! run_sysroot_command( "setup", &command ) )
    {
        goto fail;
    }

    if ( ! g_ctx.dryrun && ! sysroot_exists() )
    {
        SOLOG( ERR, "Sysroot directory '%s' does not exist after provisioning", first( &dir ) );
        goto fail;
    }

    cleanup( &command );
    cleanup( &base );
    cleanup( &dir );
    sysroot_archive_cleanup( &archive );
    return;

fail:
    cleanup( &command );
    cleanup( &base );
    cleanup( &dir );
    sysroot_archive_cleanup( &archive );
    exit( EXIT_FAILURE );
}
