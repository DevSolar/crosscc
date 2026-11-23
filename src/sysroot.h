#ifndef CROSSCC_SRC_SYSROOT_H
#define CROSSCC_SRC_SYSROOT_H

#include "support/cc_include.h"

#include <stdbool.h>

/* Returns true if the expanded sysroot.dir setting names an existing,
 * readable directory.
 */
bool sysroot_exists( void );

typedef struct
{
    string basename;
    string path;
} sysroot_archive_t;

/* Resolves the archive path for sysroot.url within the configured
 * settings.cache search path.
 * If an existing readable archive is found, returns it in .path with
 * an empty .basename. Otherwise returns the first cache directory in
 * .path and the archive filename in .basename.
 */
sysroot_archive_t sysroot_cache_file( void );

/* Verifies the SHA-256 checksum of 'archive' against 'expected'
 * (hex-encoded). Returns true if 'expected' is NULL/empty or matches
 * the computed digest, false on mismatch or read error.
 */
bool sysroot_verify_checksum( char const * archive, char const * expected );

/* Downloads the configured sysroot archive
 * Exits with EXIT_FAILURE on unrecoverable errors.
 */
void sysroot_archive_download( sysroot_archive_t * archive );

/* Ensures the target sysroot is provisioned. If sysroot_exists() is
 * false, downloads the archive (if not already cached), verifies
 * sysroot.checksum (if set), unpacks into sysroot.base via
 * sysroot.unpack, and runs sysroot.setup (if set). Respects
 * g_ctx.dryrun by logging actions without filesystem side effects.
 * Exits with EXIT_FAILURE on unrecoverable errors.
 */
void provision_sysroot( void );

#endif
