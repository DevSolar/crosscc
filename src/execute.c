#include "execute.h"

#include "context.h"
#include "support/cc_include.h"
#include "support/solog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#else
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
extern char ** environ;
#endif

#ifdef _WIN32
static int run_child( char ** argv )
{
    int code = (int)_spawnvp( _P_WAIT, argv[0], (char const * const *)argv );
    if ( code < 0 )
    {
        SOLOG( ERR, "Failed to execute binary '%s'", argv[0] );
        return EXIT_FAILURE;
    }
    return code;
}
#else
static int run_child( char ** argv )
{
    pid_t pid;
    int err;
    int status;

    err = posix_spawnp( &pid, argv[0], NULL, NULL, argv, environ );
    if ( err != 0 )
    {
        SOLOG( ERR, "Failed to spawn binary '%s': %s", argv[0], strerror( err ) );
        return EXIT_FAILURE;
    }

    status = 0;
    if ( waitpid( pid, &status, 0 ) != -1 )
    {
        if ( WIFEXITED( status ) )
        {
            return WEXITSTATUS( status );
        }
        else if ( WIFSIGNALED( status ) )
        {
            return 128 + WTERMSIG( status );
        }
    }
    return EXIT_FAILURE;
}
#endif

int execute( vec( string ) * cmdline )
{
    size_t count;
    size_t i;
    char ** argv;
    int result;

    if ( cmdline == NULL || size( cmdline ) == 0 )
    {
        SOLOG( ERR, "No command line arguments to execute." );
        return EXIT_FAILURE;
    }

    count = size( cmdline );
    argv = (char **)malloc( ( count + 1 ) * sizeof( char * ) );

    if ( argv == NULL )
    {
        SOLOG( ERR, "Memory allocation failed for argv array" );
        return EXIT_FAILURE;
    }

    for ( i = 0; i < count; ++i )
    {
        str( char ) * item = get( cmdline, i );
        argv[ i ] = first( item );
    }

    argv[ count ] = NULL;

    if ( g_ctx.dryrun )
    {
        printf( "[dryrun]" );

        for ( i = 0; i < count; ++i )
        {
            printf( " %s", argv[ i ] );
        }

        printf( "\n" );
        SOLOG( INFO, "Dry run mode active - command not executed." );
        free( argv );
        return EXIT_SUCCESS;
    }

    SOLOG( DEBUG, "Executing binary '%s' with %zu arguments", argv[0], count );

    result = run_child( argv );
    free( argv );
    return result;
}
