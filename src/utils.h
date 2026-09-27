#ifndef CROSSCC_UTILS_H
#define CROSSCC_UTILS_H

#include "support/cc_include.h"

#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    char const * begin;
    char const * end;
} strspan_t;

/* case-insensitive strcmp() */
int stricmp( char const * lhs, char const * rhs );
/* case-insensitive strncmp() */
int strnicmp( char const * lhs, char const * rhs, size_t n );
/* case-insensitive numeric compare */
int strvicmp( char const * lhs, char const * rhs );

/* return pointer to last appearance of any character in c in span,
 * or NULL if none of the characters in c are found in c
 */
char * strrpbrk( strspan_t * span, char const * c );

/* returns true if 'filename' identifies a readable, regular file */
bool file_readable( char const * filename );
/* returns true if 'dirname' identifies a readable directory */
bool dir_readable( char const * dirname );

/* trims trailing whitespace from s */
void rtrim( char * s );
/* trims trailing whitespace from str */
void rtrim_str( string * str );

/* returns pointer to the first non-whitespace character in s;
 * does not, actually, trim the string
 */
char * ltrim( char const * s );
/* trims leading whitespace from str */
void ltrim_str( string * str );

/* read line (which may be continuated by '\' prior to newline),
 * returns negative on error, zero on EOF, and line number on success
 * re-using string object for memory efficiency
 */
int get_line( FILE * fh, string * line, int * lineno );

/* splits a delimiter-separated list, and invokes a callback function
 * for each element. If the callback returns false, the loop terminates.
 */
int foreach( char const * list, char const * delims, bool (*callback)( string *, void * ), void * userdata );

/* given a list of candidates, consisting of one environment variable
 * and one pathname, the function goes through the candidate list,
 * expanding the environment variable, appending the pathname, and
 * checking if a file of that name exists. If the environment variable
 * consists of a list of paths (like PATH does), the function checks
 * all combinations. It returns the first candidate that resolves to
 * an existing file, or an empty string if none is found. Failure to
 * expand an environment variable given fails the candidate.
 */
typedef struct
{
    char const * env;
    char const * path;
} candidate_t;

string find_first( vec( candidate_t ) candidates );

/* get a setting from context; returns empty string if setting
 * is not found.
 */
string get_setting( char const * section, char const * key );

#endif
