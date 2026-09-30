#ifndef CROSSCC_CONTEXT_H
#define CROSSCC_CONTEXT_H

#include "support/cc_include.h"

#include <stdbool.h>

#define OPTIONS( X ) \
    X( build,    'b', string, "Build type (debug / release)" ) \
    X( config,   'c', string, "Config.ini override" ) \
    X( dryrun,   'd',   bool, "Perform dry run without execution" ) \
    X( help,     'h',   bool, "Display this help" ) \
    X( info,     'i',   bool, "Display info on available targets" ) \
    X( link,     'l', string, "Link style (static / shared)" ) \
    X( mode,     'm', string, "CrossCC mode (compiler, linker, ..." ) \
    X( optimize, 'o', string, "Optimization goal (speed / size)" ) \
    X( profile,  'p',   bool, "Enable profiling options" ) \
    X( sanitize, 's',   bool, "Enable sanitizing options" ) \
    X( target,   't', string, "Build target (x86_64, aarch64, ...)" ) \
    X( verbose,  'v',   long, "Set verbosity level (0=quiet, 2=debug)" )

#if defined( __clang__ )
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

typedef struct
{
#define X( name, initial, type, desc ) type name;
    OPTIONS( X )
#undef X
    vec( string ) passthrough;
    map( string, string ) settings;
} context_t;

#if defined( __clang__ )
#pragma clang diagnostic pop
#endif

extern context_t g_ctx;

void ctx_init( void );
void ctx_cleanup( void );
void dump( void );
void help( void );

#endif
