#include "config.h"

#include "context.h"
#include "support/cc_include.h"
#include "support/solog.h"
#include "utils.h"

#include <dirent.h>

#include <stdlib.h>

/* Location of the global config file */
static string global_config_file( void )
{
    char const * conf_env;
    string config_dir;

    init( &config_dir );

#ifdef _WIN32
    conf_env = getenv( "APPDATA" );

    if ( conf_env && *conf_env )
    {
        push_fmt( &config_dir, conf_env );
    }
#else
    conf_env = getenv( "XDG_CONFIG_HOME" );

    if ( conf_env && *conf_env )
    {
        push_fmt( &config_dir, conf_env );
    }
    else
    {
        char const * home = getenv( "HOME" );

        if ( home && *home )
        {
            push_fmt( &config_dir, home, "/.config" );
        }
    }
#endif

    if ( size( &config_dir ) > 0 )
    {
        push_fmt( &config_dir, "/crosscc/config.ini" );
    }

    return config_dir;
}

/* Parse [...] */
static bool parse_section( string * line, string * section )
{
    /* End of section name */
    char * s = strchr( first( line ) + 1, ']' );
    char * p;

    if ( s == NULL )
    {
        SOLOG( ERR, "Unpaired '['" );
        return false;
    }

    /* Get whatever is after the ']' */
    p = ltrim( s + 1 );

    if ( *p == '\0' || *p == ';' || *p == '#' )
    {
        /* No text, or comment only after the ']' */
        clear( section );
        push_n( section, first( line ) + 1, (size_t)( s - first( line ) - 1 ) );
        ltrim_str( section );
        rtrim_str( section );
        SOLOG( TRACE, "Section '%s'", first( section ) );
    }
    else
    {
        SOLOG( ERR, "Excess text after section declaration" );
        return false;
    }

    return true;
}

/* Parse x = y */
static bool parse_key_value( string * line, string * section )
{
    /* Pointing at the assignment */
    char * s = strpbrk( first( line ), ":=" );
    string key;
    string value;

    if ( s == NULL )
    {
        SOLOG( ERR, "'%s': No assignment", first( line ) );
        return false;
    }

    if ( s == first( line ) )
    {
        SOLOG( ERR, "'%s': Empty key", first( line ) );
        return false;
    }

    /* Splitting off key and value */
    *s++ = '\0';
    s = ltrim( s );

    /* Trimming key */
    rtrim( first( line ) );

    if ( *first( line ) == '\0' )
    {
        SOLOG( ERR, "'%s': Empty key", first( line ) );
        return false;
    }

    if ( *s == '"' || *s == '\'' )
    {
        /* Quoted value */
        char * p = strchr( s + 1, *s );

        if ( ! p )
        {
            SOLOG( ERR, "'%s': Unpaired '%c'", s, *s );
            return false;
        }

        /* Getting the quoted value (no quote escaping) */
        ++s;
        *p = '\0';

        p = ltrim( p + 1 );

        if ( *p && *p != ';' && *p != '#' )
        {
            SOLOG( ERR, "'%s': Excess text after quoted value", s );
            return false;
        }
    }
    else
    {
        char * p = strpbrk( s, ";#" );

        if ( p )
        {
            *p = '\0';
        }

        rtrim( s );
    }

    init( &key );

    if ( size( section ) > 0 )
    {
        push_fmt( &key, first( section ), "." );
    }

    push_fmt( &key, first( line ) );

    init( &value );

    if ( *s )
    {
        push_fmt( &value, s );
    }

    SOLOG( TRACE, "Insert {%s}, {%s}", first( &key ), first( &value ) );
    insert( &g_ctx.settings, key, value );
    return true;
}

/* Parse .ini file */
static void parse( char const * filename )
{
    FILE * fh;
    int lineno = 0;
    string section;
    string line;
    bool fail = false;

    init( &section );
    init( &line );

    if ( ! filename || ! *filename || ( fh = fopen( filename, "r" ) ) == NULL )
    {
        SOLOG( ERR, "Could not read config file." );
        exit( EXIT_FAILURE );
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
                    fail = true;
                    goto end;
                }
                break;
            default:
                if ( ! parse_key_value( &line, &section ) )
                {
                    SOLOG( ERR, "Malformed .ini file: '%s', line %d", filename, lineno );
                    fail = true;
                    goto end;
                }
                break;
        }
    }

end:
    fclose( fh );
    cleanup( &section );
    cleanup( &line );

    if ( fail )
    {
        exit( EXIT_FAILURE );
    }
}

/* Determine main .ini file, parse */
void load_ini( void )
{
    string ini_file;

    if ( size( &g_ctx.config ) > 0 )
    {
        init_clone( &ini_file, &g_ctx.config );
    }
    else if ( file_readable( "crosscc.ini" ) )
    {
        init( &ini_file );
        push_fmt( &ini_file, "crosscc.ini" );
    }
    else
    {
        ini_file = global_config_file();
    }

    if ( ! file_readable( first( &ini_file ) ) )
    {
        SOLOG( ERR, "Configuration file '%s' not found.", first( &ini_file ) );
        cleanup( &ini_file );
        exit( EXIT_FAILURE );
    }

    parse( first( &ini_file ) );
    cleanup( &ini_file );
}

/* Parse target .ini file */
static bool load_target_ini_cb( string * dir, void * udata )
{
    string * ini_file = (string *)udata;
    string path;
    init( &path );

    if ( size( dir ) > 0 )
    {
        push_fmt( &path, *dir );

        if ( *last( &path ) != '/' && *last( &path ) != '\\' )
        {
            push( &path, '/' );
        }
    }

    push_fmt( &path, first( &g_ctx.target ), ".ini" );

    if ( file_readable( first( &path ) ) )
    {
        push_fmt( ini_file, path );
    }

    cleanup( &path );
    return size( ini_file ) == 0;
}

void load_target_ini( void )
{
    string inifiles;
    string ini_file;

    if ( size( &g_ctx.target ) > 0 )
    {
        init( &ini_file );

        inifiles = get_setting_expanded( "settings", "inifiles" );
        foreach( first( &inifiles ), ";", load_target_ini_cb, &ini_file );

        if ( size( &ini_file ) > 0 )
        {
            parse( first( &ini_file ) );
        }
        else
        {
            SOLOG( ERR, "'%s.ini' not found in any of '%s'.", first( &g_ctx.target ), first( &inifiles ) );
            cleanup( &ini_file );
            cleanup( &inifiles );
            exit( EXIT_FAILURE );
        }

        cleanup( &ini_file );
        cleanup( &inifiles );
    }
    else
    {
        SOLOG( ERR, "No target specified." );
        exit( EXIT_FAILURE );
    }
}

/* List all targets for which .ini files are available */
static bool list_targets_cb( string * dir, void * udata )
{
    DIR * d = opendir( first( dir ) );
    (void)udata;

    if ( d )
    {
        struct dirent * ent;
        string path;
        init( &path );

        while ( ( ent = readdir( d ) ) != NULL )
        {
            size_t nlen = strlen( ent->d_name );
            if ( nlen > 4 && strcmp( ent->d_name + nlen - 4, ".ini" ) == 0 )
            {
                clear( &path );

                if ( size( dir ) > 0 )
                {
                    push_fmt( &path, *dir );

                    if ( *last( &path ) != '/' && *last( &path ) != '\\' )
                    {
                        push( &path, '/' );
                    }
                }

                push_fmt( &path, ent->d_name );

                if ( file_readable( first( &path ) ) )
                {
                    printf( "%.*s\n", (int)( nlen - 4 ), ent->d_name );
                }
            }
        }

        cleanup( &path );
        closedir( d );
    }
    return true;
}

void list_targets( void )
{
    string inifiles = get_setting_expanded( "settings", "inifiles" );
    foreach( first( &inifiles ), ";", list_targets_cb, NULL );
    cleanup( &inifiles );
}
