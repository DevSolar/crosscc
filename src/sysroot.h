#ifndef CROSSCC_SRC_SYSROOT_H
#define CROSSCC_SRC_SYSROOT_H

#include "support/cc_include.h"

#include <stdbool.h>

/* Returns true if the expanded sysroot.dir setting names an existing,
 * readable directory.
 */
bool sysroot_exists( void );

/* Resolves the destination archive path for sysroot.url within the
 * configured settings.cache search path (falling back to
 * {ENV:XDG_CACHE_HOME}/crosscc or {ENV:HOME}/.cache/crosscc).
 * Returns an existing cached file if found, or the path in the first
 * cache directory where the archive should be downloaded.
 */
string sysroot_cache_file( void );

/* Verifies the SHA-256 checksum of 'archive' against 'expected'
 * (hex-encoded). Returns true if 'expected' is NULL/empty or matches
 * the computed digest, false on mismatch or read error.
 */
bool sysroot_verify_checksum( char const * archive, char const * expected );

/* Ensures the target sysroot is provisioned. If sysroot_exists() is
 * false, downloads the archive (if not already cached), verifies
 * sysroot.checksum (if set), unpacks into sysroot.base via
 * sysroot.unpack, and runs sysroot.setup (if set). Respects
 * g_ctx.dryrun by logging actions without filesystem side effects.
 * Exits with EXIT_FAILURE on unrecoverable errors.
 */
void provision_sysroot( void );

#endif
