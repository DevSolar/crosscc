#include "config.h"

#include "support/cc_include.h"
#include "support/solog.h"
#include "utils.h"

#include <stdlib.h>

static string global_config_file( void )
{
    char const * conf_env;
    string config_dir;

    init( &config_dir );

#ifdef _WIN32
    conf_env = getenv( "APPDATA" );

    push_fmt( &config_dir, conf_env );
#else
    conf_env = getenv( "XDG_CONFIG_HOME" );

    if ( conf_env )
    {
        push_fmt( &config_dir, conf_env );
    }
    else
    {
        char const * home = getenv( "HOME" );
        push_fmt( &config_dir, home, "/.config" );
    }
#endif

    push_fmt( &config_dir, "/crosscc/config.ini" );

    return config_dir;
}

static bool parse_section( string * line, string * section )
{
    (void)line;
    (void)section;
    // t.b.d.
    return true;
}

static bool parse_key_value( string * line, string * section )
{
    (void)line;
    (void)section;
    // t.b.d.
    return true;
}

static void parse( char const * filename )
{
    FILE * fh;
    int lineno = 0;
    string section;
    string line;

    init( &section );
    init( &line );

    if ( ! filename || ! *filename || ( fh = fopen( filename, "r" ) ) == NULL )
    {
        SOLOG( ERR, "Could not read config file." );
        return;
    }

    while ( get_line( fh, &line, &lineno ) > 0 )
    {
        switch ( *first( &line ) )
        {
            case '#':
            case ';':
            case '\0':
                continue;
            case '[':
                if ( ! parse_section( &line, &section ) )
                {
                    SOLOG( ERR, "Malformed .ini file: '%s', line %d", filename, lineno );
                    fclose( fh );
                    cleanup( &section );
                    cleanup( &line );
                    exit( EXIT_FAILURE );
                }
                break;
            default:
                if ( ! parse_key_value( &line, &section ) )
                {
                    SOLOG( ERR, "Malformed .ini file: '%s', line %d", filename, lineno );
                    fclose( fh );
                    cleanup( &section );
                    cleanup( &line );
                    exit( EXIT_FAILURE );
                }
                break;
        }
    }

    fclose( fh );
    cleanup( &section );
    cleanup( &line );
}

void load_cfg( void )
{
    string cfg_file;

    if ( file_readable( "crosscc.ini" ) )
    {
        init( &cfg_file );
        push_fmt( &cfg_file, "crosscc.ini" );
    }
    else
    {
        cfg_file = global_config_file();

        if ( ! file_readable( first( &cfg_file ) ) )
        {
            SOLOG( ERR, "No configuration file found." );
            cleanup( &cfg_file );
            exit( EXIT_FAILURE );
        }
    }

    parse( first( &cfg_file ) );
    cleanup( &cfg_file );
}

void load_target_cfg( void )
{
    // t.b.d.
}

void list_targets( void )
{
    // t.b.d.
}
