#ifndef CROSSCC_TOOLS_H
#define CROSSCC_TOOLS_H

#include "support/cc_include.h"

#include <stdbool.h>

/* Streams each provided file to stdout in 64 KB chunks using standard
 * fopen("rb") / fread() / fwrite().
 */
bool concat( vec( string ) * options );

/* Copies a source file to destination. Respects --if-newer via is_newer().
 * - Win32: CopyFileA(src, dst, FALSE).
 * - POSIX: Chunked copy with chmod() to replicate source file permissions.
 */
bool copy( vec( string ) * options );

/* Recursively traverses directories, copying nested subdirectories and files.
 * - Win32: Traversal via FindFirstFileA() / FindNextFileA(), directory
 *          creation via CreateDirectoryA().
 * - POSIX: Traversal via opendir() / readdir(), directory creation via mkdir().
 */
bool copydir( vec( string ) * options );

/* Prints arguments separated by spaces, followed by a newline and fflush(stdout).
 */
bool echo( vec( string ) * options );

/* Unsets environment variables.
 * - Win32: SetEnvironmentVariableA(name, NULL).
 * - POSIX: unsetenv(name).
 */
bool unset_env( vec( string ) * options );

/* Sets environment variable (supports "KEY=VALUE" or "KEY" "VALUE").
 * - Win32: SetEnvironmentVariableA(key, val).
 * - POSIX: setenv(key, val, 1).
 */
bool set_env( vec( string ) * options );

/* Creates directory paths recursively (equivalent to mkdir -p).
 * Walks path components, ignoring existing directories (EEXIST / ERROR_ALREADY_EXISTS).
 * - Win32: CreateDirectoryA().
 * - POSIX: mkdir(path, 0777).
 */
bool makedir( vec( string ) * options );

/* Self-contained, portable SHA-256 implementation (zero external library or
 * linker dependencies). Reads the file in 64 KB blocks and prints the
 * 64-character lowercase hex digest and filename to stdout.
 */
bool checksum( vec( string ) * options );

/* Removes files or directories. Supports --force (ignore missing files) and
 * --all (recursive directory tree deletion).
 * - Win32: Files removed via DeleteFileA(); directories traversed and removed
 *          via RemoveDirectoryA().
 * - POSIX: Files removed via unlink(); directories traversed and removed
 *          via rmdir().
 */
bool rm( vec( string ) * options );

/* Suspends execution for given duration in seconds. Parses fractional seconds
 * via strtod().
 * - Win32: Sleep((DWORD)(secs * 1000.0)).
 * - POSIX: nanosleep(&req, &req) with an EINTR retry loop.
 */
bool sleep( vec( string ) * options );

/* Updates access and modification timestamps to the current time via _utime()
 * on Win32 and utime() on POSIX. Supports --create to create non-existent files
 * via fopen(path, "ab").
 */
bool touch( vec( string ) * options );

/* Creates a link pointing destination to source.
 * - Win32: Attempts CreateSymbolicLinkA(), falls back to CreateHardLinkA(),
 *          and falls back to copying the file if linking is unsupported.
 * - POSIX: Attempts symlink(), falls back to linkat(), and falls back to
 *          copying the file.
 */
bool link( vec( string ) * options );

/* Extracts an archive file (.tar, .tar.gz, .tgz, .zip, etc.).
 * Calls tar -xf <archive> [-C <dest>] via execute(), supported natively across
 * both modern Windows 10/11 (tar.exe) and POSIX.
 */
bool unpack( vec( string ) * options );

#endif
