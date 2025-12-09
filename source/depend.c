#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <curl/curl.h>

#include "extra.h"
#include "utils.h"
#include "curl.h"
#include "archive.h"
#include "crypto.h"
#include "units.h"
#include "depend.h"

/* Global buffers for dependency management operations */
char package_command[ WG_MAX_PATH * 4 ]                 ;    /* Buffer for system commands */
char *tag                                               ;    /* Version tag for dependencies */
char *path                                              ;    /* Path component of dependency URL */
char *path_slash                                        ;    /* Pointer to path separator */
char *user                                              ;    /* Repository owner/username */
char *repo                                              ;    /* Repository name */
char *repo_slash                                        ;    /* Pointer to repository separator */
char *git_dir                                           ;    /* Pointer to .git extension */
char *filename                                          ;    /* Extracted filename */
char *extension                                         ;    /* File extension */
char json_item[WG_PATH_MAX]                             ;    /* Json item's */
static int found_cnt = 0                                ;    /* dump file counts */

/*
 * This is the maximum dependency in 'watchdogs.toml' that can be read from the 'packages' key,
 * and you need to ensure this capacity is more than sufficient. If you enter 101, it will only read up to 100.
*/
#define MAX_DEPENDS (102)

struct dency_repositories {
        char
                host[32]; /* repo host */
        char
                domain[64]; /* repo domain */
        char
                user[128]; /* repo user */
        char
                repo[128]; /* repo name */
        char
                tag[128]; /* repo tag */
};

typedef struct {
        const char
                *dency_config; /* depends config */
        const char
                *dency_target; /* depends target */
        const char
                *dency_added; /* depends packages */
} dencyconfig;

const char* root_keywords[/* keywords root lib */] = {
        "lib",
        "log",
        "root",
        "amx",
        "static",
        "dynamic",
        "cfg",
        "config",
        "msvcrt",
        "msvcr",
        "msvcp",
        "msvcp",
        "ucrtbase",
        "vcruntime"
};

const char * matching_operating_system[] = {
        "windows",
        "win",
        "win32",
        "msvc",
        "mingw",
        "linux",
        "ubuntu",
        "debian",
        "cent",
        "centos",
        "cent_os",
        "fedora",
        "arch",
        "archlinux",
        "alphine"
        "rhel",
        "redhat",
        "linuxmint",
        "mint"
};

const char *windows_patterns[] = {
        "windows",
        "win",
        "win32",
        "win32",
        "msvc",
        "mingw",
        ".dll",
        NULL
};
    
const char *linux_patterns[] = {
        "linux",
        "ubuntu",
        "debian",
        "cent",
        "centos",
        "cent_os",
        "fedora",
        "arch",
        "archlinux",
        "alphine"
        "rhel",
        "redhat",
        "linuxmint",
        "mint",
        ".so",
        NULL
};

const char * matching_any_patterns[] = {
        "src",
        "source",
        "proj",
        "project",
        "server",
        "_server",
        "gamemode",
        "gamemodes",
        "bin",
        "build",
        "packages",
        "resources",
        "modules",
        "plugins",
        "addons",
        "extensions",
        "scripts",
        "system",
        "core",
        "runtime",
        "libs",
        "include",
        "deps",
        "dependencies"
};

/* Path Separator */
static inline const char * PATH_SEPARATOR( const char *path ) {
        const char *l = strrchr( path, __PATH_CHR_SEP_LINUX );
        const char *w = strrchr( path, __PATH_CHR_SEP_WIN32 );
        if ( l && w )
                return ( l > w ) ? l : w;
        return l ? l : w;
}

/*
 * this_os_archive - check if filename indicates OS-specific archive
 *
 * Identifies platform-specific files by name patterns. Returns 1 if the
 * filename contains any OS pattern, 0 otherwise.
 */
int this_os_archive( const char *filename )
{
        int k;
        
        /* Determine target operating system at compile time */
        const char *target_os = NULL;
        #ifdef WG_WINDOWS
        target_os = "windows"; //Compile-time flag for Windows builds
        #else
        target_os = "linux";   //Default to Linux if not Windows
        #endif
        
        if ( !target_os ) return 0; //No target OS specified
        
        /* Convert target_os to lowercase for case-insensitive comparison */
        char target_lower[ WG_PATH_MAX ];
        strncpy( target_lower, target_os, sizeof ( target_lower ) - 1 );
        target_lower[ sizeof( target_lower ) - 1 ] = '\0';
        
        for (int i = 0; target_lower[ i ]; i++) {
            target_lower[ i ] = tolower(target_lower[ i ]);
        }
        
        /* Select OS-specific patterns */
        const char **patterns = NULL;
        if (strstr( target_lower, "win" )) {
            patterns = windows_patterns;
        } else if (strstr( target_lower, "linux")) {
            patterns = linux_patterns;
        }
        
        if ( !patterns ) return 0;
        
        /* Convert filename to lowercase for case-insensitive matching */
        char filename_lower[ WG_PATH_MAX ];
        strncpy( filename_lower,
                filename,
                sizeof ( filename_lower ) - 1 );
        filename_lower [sizeof ( filename_lower ) - 1 ] = '\0';
        
        for (int i = 0; filename_lower[ i ]; i++) {
            filename_lower[ i ] = tolower(filename_lower[ i ]);
        }
        
        /* Check for patterns */
        for (k = 0; patterns[ k ] != NULL; ++k) {
            if ( strstr(
                    filename_lower,
                    patterns[ k ] )
                ) {
                return 1; // Match found
            }
        }
        
        return 0; //No match found
}

/*
 * this_more_archive - check if filename indicates from "root_keywords" archive
 *
 * Identifies server-related files by name patterns. Returns 1 if the
 * filename contains any server pattern, 0 otherwise.
 */
int this_more_archive(const char *filename)
{
        int k;                     /* loop counter for array index */
        int ret = 0;               /* return value, default 0 (not found) */

        /* Iterate through all server patterns until NULL terminator */
        for (k = 0;
             matching_any_patterns[k] != NULL;  /* check array end */
             ++k) {
        	/* Search for current server pattern in filename */
        	if (strfind(
                        filename,               /* string to search in */
                        matching_any_patterns[k],  /* pattern to find */
                        true)                   /* case-sensitive flag */
        	) {
                        ret = 1;    /* pattern found, set return to 1 */
                        break;      /* exit loop early */
                }
        }

        return ret;  /* return result (0=not found, 1=found) */
}

/*
 * dency_fetching_assets - select most appropriate asset from available options
 *
 * Prioritizes assets in the following order:
 * 1. Server/gamemode assets for target OS
 * 2. OS-specific assets for target OS
 * 3. Server/gamemode assets (any OS)
 * 4. Neutral assets (no OS pattern)
 * 5. First asset as fallback
 *
 * Returns a newly allocated string containing the selected asset filename,
 * or NULL if no assets are available.
 */
char *dency_fetching_assets(char **package_assets,
                            int counts, const char *preferred_os)
{
        int i;
        
        /* Handle edge cases */
        if (counts == 0)
            return NULL;
        if (counts == 1)
            return strdup(package_assets[ 0 ]);
        
        /* Determine target OS - use preferred_os if provided, otherwise detect from compile flags */
        const char *target_os = NULL;
        if (preferred_os && preferred_os[ 0 ]) {
            target_os = preferred_os;
        } else {
            #ifdef WG_WINDOWS
            target_os = "windows";
            #else
            target_os = "linux";
            #endif
        }
        
        /* Convert target_os to lowercase for comparison */
        char target_lower[ 32 ] = { 0 };
        if (target_os) {
            strncpy( target_lower, target_os, sizeof(target_lower) - 1 );
            for (int j = 0; target_lower[j]; j++) {
                target_lower[j] = tolower(target_lower[j]);
            }
        }
        
        /* Priority 1: Server/gamemode assets for target OS */
        if (target_lower[ 0 ]) {
            for (i = 0; i < counts; i++) {
                /* Check the assets from any patterns */
                int is_server_asset = 0;
                for (int p = 0; matching_any_patterns[ p ] != NULL; p++) {
                    if (strfind(package_assets[ i ], matching_any_patterns[ p ], true)) {
                        is_server_asset = 1;
                        break;
                    }
                }
                
                if (is_server_asset) {
                    /* Check if it's also for our target OS */
                    if (strstr(target_lower, "win")) {
                        /* Check for Windows patterns */
                        for (int w = 0; windows_patterns[ w ] != NULL; w++)
                        {
                            if ( strstr(package_assets[ i ],
                                windows_patterns[ w ] )
                            ) {
                                return
                                        strdup( package_assets [ i ] );
                            }
                        }
                    } else if (strstr(target_lower, "linux")) {
                        /* Check for Linux patterns */
                        for (int l = 0; linux_patterns[ l ] != NULL; l++)
                        {
                            if (strstr( package_assets[ i ],
                                linux_patterns[ l ])
                            ) {
                                return strdup( package_assets [ i ] );
                            }
                        }
                    }
                }
            }
        }
        
        /* Priority 2: OS-specific assets for target OS */
        if (target_lower[ 0 ]) {
            for (i = 0; i < counts; i++) {
                if (strstr( target_lower, "win") ) {
                    /* Check for Windows patterns */
                    for (int w = 0; windows_patterns[ w ] != NULL; w++)
                    {
                        if (strstr(package_assets[ i ],
                            windows_patterns[ w ]))
                        {
                            return
                                strdup( package_assets [ i ] );
                        }
                    }
                } else if ( strstr(target_lower, "linux") ) {
                    /* Check for Linux patterns */
                    for (int l = 0; linux_patterns[ l ] != NULL; l++)
                    {
                        if (strstr(package_assets[ i ],
                            linux_patterns[ l ])) {
                            return
                                strdup( package_assets [ i ] );
                        }
                    }
                }
            }
        }
        
        /* Priority 3: Server/gamemode assets (any OS) */
        for (i = 0; i < counts; i++) {
            for (int p = 0; matching_any_patterns[ p ] != NULL; p++) {
                if ( strfind(package_assets[ i ],
                    matching_any_patterns[ p ], true) )
                {
                    return
                        strdup( package_assets [ i ] );
                }
            }
        }
        
        /* Priority 4: Neutral assets (no OS pattern) */
        for (i = 0; i < counts; i++) {
            int rate_os_pattern = 0;
            
            /* Check for Windows patterns */
            for (int w = 0; windows_patterns[ w ] != NULL && !rate_os_pattern; w++) {
                if ( strstr(package_assets[ i ],
                    windows_patterns[ w ]) )
                {
                    rate_os_pattern = 1;
                }
            }
            
            /* Check for Linux patterns */
            for (int l = 0; linux_patterns[ l ] != NULL && ! rate_os_pattern; l++) {
                if ( strstr(package_assets[ i ],
                    linux_patterns[ l ]) )
                {
                    rate_os_pattern = 1;
                }
            }
            
            if ( ! rate_os_pattern ) {
                return
                        strdup( package_assets [ i ] );
            }
        }
        
        /* Fallback: First asset */
        return
                strdup( package_assets [ 0 ] );
}

/* 
 * Convert Windows path separators to Unix/Linux style
 * Replaces backslashes with forward slashes for cross-platform compatibility
 */
void dency_sym_convert( char *path )
{
        char *pos;
        for ( pos = path; *pos; pos++ ) {
                if ( *pos == __PATH_CHR_SEP_WIN32 )
                        *pos = __PATH_CHR_SEP_LINUX;
        }
}

/* 
 * Extract filename from full path (excluding directory)
 * Returns pointer to filename component
 */
const char *dency_get_filename(
        const char *path
) {
        const char *dependencies = PATH_SEPARATOR( path );
        return dependencies ?
               dependencies + 1 : path;
}

/* 
 * Extract basename from full path (similar to dency_get_filename)
 * Alternative implementation for filename extraction
 */
static const char *dency_get_basename(
        const char *path
) {
        const char *p = PATH_SEPARATOR( path );
        return p ?
               p + 1 : path;
}

/* 
 * Parse repository URL/identifier into structured components
 * Extracts host, domain, user, repo, and tag from dependency string
 */
static int
dency_parse_repo( const char *input, struct dency_repositories *__package_data )
{
        memset( __package_data, 0, sizeof( *__package_data ) );

        /* Default to GitHub */
        strcpy( __package_data->host, "github" );
        strcpy( __package_data->domain, "github.com" );

        char parse_input[WG_PATH_MAX * 2];
        strncpy( parse_input, input, sizeof( parse_input ) - 1 );
        parse_input[sizeof( parse_input ) - 1] = '\0';

        /* Extract tag (version) if present (format: user/repo:tag) */
        tag = strrchr( parse_input, '?' );
        if ( tag ) {
                *tag = '\0';
                strncpy( __package_data->tag, tag + 1, sizeof( __package_data->tag ) - 1 );

                if ( !strcmp( __package_data->tag, "newer" ) ) {
                        printf( "Spot " );
                        pr_color( stdout, FCOLOUR_CYAN, "[?newer] " );
                        printf( "tag, rolling with the freshest release..\t\t[All good]\n" );
                }
        }

        path = parse_input;

        /* Strip protocol prefix */
        if ( !strncmp( path, "https://", 8 ) )
                path += 8;
        else if ( !strncmp( path, "http://", 7 ) )
                path += 7;

        /* Handle shorthand GitHub format */
        if ( !strncmp( path, "github/", 7 ) ) {
                strcpy( __package_data->host, "github" );
                strcpy( __package_data->domain, "github.com" );
                path += 7;
        } else {
                /* Parse custom domain format (e.g., gitlab.com/user/repo) */
                path_slash = strchr( path, __PATH_CHR_SEP_LINUX );
                if ( path_slash && strchr( path, '.' ) && strchr( path, '.' ) < path_slash ) {
                        char domain[128];

                        strncpy( domain, path, path_slash - path );
                        domain[path_slash - path] = '\0';

                        if ( strstr( domain, "github" ) ) {
                                strcpy( __package_data->host, "github" );
                                strcpy( __package_data->domain, "github.com" );
                        } else {
                                strncpy( __package_data->domain, domain,
                                        sizeof( __package_data->domain ) - 1 );
                                strcpy( __package_data->host, "custom" );
                        }

                        path = path_slash + 1;
                }
        }

        /* Extract user and repository */
        user = path;
        repo_slash = strchr( path, __PATH_CHR_SEP_LINUX );
        if ( !repo_slash )
                return 0;

        *repo_slash = '\0';
        repo = repo_slash + 1;

        strncpy( __package_data->user, user, sizeof( __package_data->user ) - 1 );

        /* Remove .git extension if present */
        git_dir = strstr( repo, ".git" );
        if ( git_dir )
                *git_dir = '\0';
        strncpy( __package_data->repo, repo, sizeof( __package_data->repo ) - 1 );

        if ( strlen( __package_data->user ) == 0 || strlen( __package_data->repo ) == 0 )
                return 0;

#if defined( _DBG_PRINT )
        /* Debug output: display parsed repository information */
        char size_title[WG_PATH_MAX * 3];
        snprintf( size_title, sizeof( size_title ), "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
                __package_data->host,
                __package_data->domain,
                __package_data->user,
                __package_data->repo,
                __package_data->tag[0] ? __package_data->tag : "(none)" );
        wg_console_title( size_title );
#endif

        return 1;
}

/* 
 * Fetch release asset URLs from GitHub repository
 * Retrieves downloadable assets for a specific release tag
 */
static int dency_gh_release_assets( const char *user, const char *repo,
                                 const char *tag, char **out_urls, int max_urls )
{
        char api_url[WG_PATH_MAX * 2];
        char *json_data = NULL;
        const char *p;
        int url_count = 0;

        /* Build GitHub API URL for specific release tag */
        snprintf( api_url, sizeof( api_url ),
                         "%sapi.github.com/repos/%s/%s/releases/tags/%s",
                         "https://", user, repo, tag );

        /* Fetch release information JSON */
        if ( !dency_http_get_content( api_url, wgconfig.wg_toml_github_tokens, &json_data ) )
                return 0;

        /* Parse JSON to extract browser_download_url entries */
        p = json_data;
        while ( url_count < max_urls && ( p = strstr( p, "\"browser_download_url\"" ) ) != NULL ) {
                const char *url_end;
                size_t url_len;

                p += strlen( "\"browser_download_url\"" );
                p = strchr( p, '"' );
                if ( !p )
                        break;
                ++p;

                url_end = strchr( p, '"' );
                if ( !url_end )
                        break;

                url_len = url_end - p;
                out_urls[url_count] = wg_malloc( url_len + 1 );
                if ( !out_urls[url_count] ) {
                        chain_ret_main( NULL );
                }
                strncpy( out_urls[url_count], p, url_len );
                out_urls[url_count][url_len] = '\0';

                ++url_count;
                p = url_end + 1;
        }

        wg_free( json_data );
        return url_count;
}

/* 
 * Construct repository URL based on parsed information
 * Builds appropriate URL for GitHub releases, tags, or branches
 */
static void
dency_build_repo_url( const struct dency_repositories *__package_data, int is_tag_page,
                   char *package_put_url, size_t package_put_size )
{
        char package_actual_tag[128] = { 0 };

        if ( __package_data->tag[0] ) {
                strncpy( package_actual_tag, __package_data->tag,
                        sizeof( package_actual_tag ) - 1 );

                /* Handle "newer" tag special case */
                if ( strcmp( package_actual_tag, "newer" ) == 0 ) {
                        if ( strcmp( __package_data->host, "github" ) == 0 && !is_tag_page ) {
                                strcpy( package_actual_tag, "latest" );
                        }
                }
        }

        /* GitHub-specific URL construction */
        if ( strcmp( __package_data->host, "github" ) == 0 ) {
                if ( is_tag_page && package_actual_tag[0] ) {
                        /* Tag page URL (for display/verification) */
                        if ( !strcmp( package_actual_tag, "latest" ) ) {
                                snprintf( package_put_url, package_put_size,
                                        "https://%s/%s/%s/releases/latest",
                                        __package_data->domain,
                                        __package_data->user,
                                        __package_data->repo );
                        } else {
                                snprintf( package_put_url, package_put_size,
                                        "https://%s/%s/%s/releases/tag/%s",
                                        __package_data->domain,
                                        __package_data->user,
                                        __package_data->repo,
                                        package_actual_tag );
                        }
                } else if ( package_actual_tag[0] ) {
                        /* Direct archive URL for specific tag */
                        if ( !strcmp( package_actual_tag, "latest" ) ) {
                                snprintf( package_put_url, package_put_size,
                                        "https://%s/%s/%s/releases/latest",
                                        __package_data->domain,
                                        __package_data->user,
                                        __package_data->repo );
                        } else {
                                snprintf( package_put_url, package_put_size,
                                        "https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
                                        __package_data->domain,
                                        __package_data->user,
                                        __package_data->repo,
                                        package_actual_tag );
                        }
                } else {
                        /* Default to main branch ZIP */
                        snprintf( package_put_url, package_put_size,
                                "https://%s/%s/%s/archive/refs/heads/main.zip",
                                __package_data->domain,
                                __package_data->user,
                                __package_data->repo );
                }
        }

#if defined( _DBG_PRINT )
        /* Debug output: display constructed URL */
        pr_info( stdout, "Built URL: %s (is_tag_page=%d, tag=%s)",
                package_put_url, is_tag_page,
                package_actual_tag[0] ? package_actual_tag : "(none)" );
#endif
}

/* 
 * Fetch latest release tag from GitHub repository
 * Uses GitHub API to get the most recent release tag
 */
static int dency_gh_latest_tag( const char *user, const char *repo,
                             char *out_tag, size_t package_put_size )
{
        char api_url[WG_PATH_MAX * 2];
        char *json_data = NULL;
        const char *p;
        int ret = 0;

        /* Build GitHub API URL for latest release */
        snprintf( api_url, sizeof( api_url ),
                        "%sapi.github.com/repos/%s/%s/releases/latest",
                        "https://", user, repo );

        /* Fetch latest release information */
        if ( !dency_http_get_content( api_url, wgconfig.wg_toml_github_tokens, &json_data ) )
                return 0;

        /* Parse JSON to extract tag_name field */
        p = strstr( json_data,
                           "\"tag_name\"" );
        if ( p ) {
                p = strchr( p, ':' );
                if ( p ) {
                        ++p; // skip colon
                        /* Skip whitespace */
                        while ( *p &&
                                   ( *p == ' ' ||
                                   *p == '\t' ||
                                   *p == '\n' ||
                                   *p == '\r' )
                                  )
                                  ++p;

                        if ( *p == '"' ) {
                                ++p; // skip opening quote
                                const char *end = strchr( p, '"' );
                                if ( end ) {
                                        size_t tag_len = end - p;
                                        if ( tag_len < package_put_size ) {
                                                strncpy( out_tag, p, tag_len );
                                                out_tag[tag_len] = '\0';
                                                ret = 1;
                                        }
                                }
                        }
                }
        }

        wg_free( json_data );
        return ret;
}

/* 
 * Process repository information to determine download URL
 * Handles latest tag resolution, asset selection, and fallback strategies
 */
static int dency_handle_repo( const struct dency_repositories *dency_repositories,
                        char *package_put_url,
                        size_t package_put_size,
                        const char *dependencies_branch )
{
        int ret = 0;
        const char *package_repo_branch[] = { dependencies_branch, "main", "master" }; // Common branch name
        char package_actual_tag[128] = { 0 };
        int use_fallback_branch = 0;

        /* Handle "latest" tag by resolving to actual tag name */
        if ( dency_repositories->tag[0] && strcmp( dency_repositories->tag, "newer" ) == 0 ) {
            if ( dency_gh_latest_tag( dency_repositories->user,
                    dency_repositories->repo,
                    package_actual_tag,
                    sizeof( package_actual_tag ) ) ) {
                printf( "Create latest tag: %s "
                        "~instead of latest (?newer)\t\t[All good]\n", package_actual_tag );
            } else {
                pr_error( stdout, "Failed to get latest tag for %s/%s,"
                        "Falling back to main branch\t\t[Fail]",
                        dency_repositories->user, dency_repositories->repo );
                use_fallback_branch = 1;
            }
        } else {
            strncpy( package_actual_tag, dency_repositories->tag, sizeof( package_actual_tag ) - 1 );
        }

        /* Fallback to main/master branch if latest tag resolution failed */
        if ( use_fallback_branch ) {
            for ( int j = 0; j < 3 && !ret; j++ ) {
                snprintf( package_put_url, package_put_size,
                        "https://github.com/%s/%s/archive/refs/heads/%s.zip",
                        dency_repositories->user, dency_repositories->repo,
                        package_repo_branch[j] );

                if ( dency_url_checking( package_put_url, wgconfig.wg_toml_github_tokens ) ) {
                    ret = 1;
                    if ( j == 1 ) printf( "Create master branch (main branch not found)\t\t[All good]\n" );
                }
            }
            return ret;
        }

        /* Process tagged releases */
        if ( package_actual_tag[0] ) {
            char *package_assets[10] = { 0 };
            int package_asset_count = dency_gh_release_assets( dency_repositories->user,
                                                    dency_repositories->repo, package_actual_tag,
                                                    package_assets, 10 );

            /* If release has downloadable assets */
            if ( package_asset_count > 0 ) {
                char *package_best_asset = NULL;
                
                /* Determine target OS based on platform defines */
                const char *target_os = NULL;
                #ifdef WG_WINDOWS
                target_os = "windows";
                #else
                target_os = "linux";
                #endif
                
                package_best_asset = dency_fetching_assets( package_assets, package_asset_count, target_os );

                if ( package_best_asset ) {
                    strncpy( package_put_url, package_best_asset, package_put_size - 1 );
                    ret = 1;
                    
                    /* asset selection */
                    if ( this_more_archive( package_best_asset ) ) {
                        ; /* empty statement */
                    } else if ( this_os_archive( package_best_asset ) ) {
                        ; /* empty statement */
                    } else {
                        ; /* empty statement */
                    }

                    pr_info(stdout, "HIT Archive: %s\t\t[All good]\n", 
                                package_best_asset );
                    
                    wg_free( package_best_asset );
                }

                /* Clean up asset array */
                for ( int j = 0; j < package_asset_count; j++ )
                    wg_free( package_assets[j] );
            }

            /* If no assets found, try standard archive formats */
            if ( !ret ) {
                const char *package_arch_format[] = {
                    "https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
                    "https://github.com/%s/%s/archive/refs/tags/%s.zip"
                };

                for ( int j = 0; j < 3 && !ret; j++ ) {
                    snprintf( package_put_url, package_put_size, package_arch_format[j],
                                    dency_repositories->user,
                                    dency_repositories->repo,
                                    package_actual_tag );

                    if ( dency_url_checking( package_put_url, wgconfig.wg_toml_github_tokens ) )
                        ret = 1;
                }
            }
        } else {
            /* No tag specified - try main/master branches */
            for ( int j = 0; j < 3 && !ret; j++ ) {
                snprintf( package_put_url, package_put_size,
                        "%s%s/%s/archive/refs/heads/%s.zip",
                        "https://github.com/",
                        dency_repositories->user,
                        dency_repositories->repo,
                        package_repo_branch[j] );

                if ( dency_url_checking( package_put_url, wgconfig.wg_toml_github_tokens ) ) {
                    ret = 1;
                    if ( j == 1 )
                        printf( "Create master branch (main branch not found)\t\t[All good]\n" );
                }
            }
        }

        return ret;
}

/* 
 * Generate and store cryptographic hash for dependency files
 * Creates SHA-1 hash for verification and tracking
 */
int dency_set_hash( const char *raw_file_path, const char *raw_json_path )
{
        char res_convert_f_path[WG_PATH_MAX];
        char res_convert_json_path[WG_PATH_MAX];

        /* Convert paths to consistent format */
        strncpy( res_convert_f_path, raw_file_path, sizeof( res_convert_f_path ) );
        res_convert_f_path[sizeof( res_convert_f_path ) - 1] = '\0';
        dency_sym_convert( res_convert_f_path );
        
        strncpy( res_convert_json_path, raw_json_path, sizeof( res_convert_json_path ) );
        res_convert_json_path[sizeof( res_convert_json_path ) - 1] = '\0';
        dency_sym_convert( res_convert_json_path );

        /* Skip hash generation for compiler directories */
        if ( strfind( res_convert_json_path, "pawno", true ) ||
                strfind( res_convert_json_path, "qawno", true ) )
                goto done;

        unsigned char sha1_hash[20];
        if ( crypto_generate_sha1_hash( res_convert_json_path, sha1_hash ) == 0 ) {
                goto done;
        }

        /* Display generated hash */
        pr_color( stdout, FCOLOUR_GREEN,
                "Create hash (SHA1) for '%s': ",
                res_convert_json_path );
        crypto_print_hex( sha1_hash, sizeof( sha1_hash ), 0 );
        printf( "\t\t[All good]\n" );

done:
        return 1;
}

/* 
 * Add plugin dependency to SA-MP server.cfg configuration
 * Updates plugins line (c_line) with new dependency
 */
void dency_implementation_samp_conf( dencyconfig config ) {
        if ( wg_server_env() != 1 )
                return;

        if (dir_exists(".watchdogs") == 0)
            MKDIR(".watchdogs");

        pr_color( stdout, FCOLOUR_GREEN,
                "Create Dependencies '%s' into '%s'\t\t[All good]\n",
                config.dency_added, config.dency_config );

        FILE* c_file = fopen( config.dency_config, "r" );
        if ( c_file ) {
                char c_line[WG_PATH_MAX];
                int t_exist = 0, tr_exist = 0, tr_ln_has_tx = 0;

                /* Read existing configuration */
                while ( fgets( c_line, sizeof( c_line ), c_file ) ) {
                        c_line[strcspn( c_line, "\n" )] = 0;
                        if ( strstr( c_line, config.dency_added ) != NULL ) {
                                t_exist = 1; /* Dependency already exists */
                        }
                        if ( strstr( c_line, config.dency_target ) != NULL ) {
                                tr_exist = 1; /* Target c_line exists */
                                if ( strstr( c_line, config.dency_added ) != NULL ) {
                                        tr_ln_has_tx = 1; /* Dependency already on target c_line */
                                }
                        }
                }
                fclose( c_file );

                if ( t_exist ) {
                        return; /* Dependency already configured */
                }

                if ( tr_exist && !tr_ln_has_tx ) {
                        /* Add to existing plugins c_line */
                        srand( ( unsigned int )time( NULL ) ^ rand() );
                        int rand7 = rand() % 10000000;

                        char temp_path[WG_PATH_MAX];
                        snprintf( temp_path, sizeof( temp_path ),
                                ".watchdogs/000%07d_temp", rand7 );

                        FILE* temp_file = fopen( temp_path, "w" );
                        c_file = fopen( config.dency_config, "r" );

                        while ( fgets( c_line, sizeof( c_line ), c_file ) ) {
                                char clean_line[WG_PATH_MAX];
                                strcpy( clean_line, c_line );
                                clean_line[strcspn( clean_line, "\n" )] = 0;

                                if ( strstr( clean_line, config.dency_target ) != NULL &&
                                        strstr( clean_line, config.dency_added ) == NULL ) {
                                        /* Append dependency to existing plugins c_line */
                                        fprintf( temp_file, "%s %s\n", clean_line, config.dency_added );
                                } else {
                                        fputs( c_line, temp_file );
                                }
                        }

                        fclose( c_file );
                        fclose( temp_file );

                        /* Replace original file with updated version */
                        remove( config.dency_config );
                        rename( ".watchdogs/depends_tmp", config.dency_config );
                } else if ( !tr_exist ) {
                        /* Create new plugins c_line */
                        c_file = fopen( config.dency_config, "a" );
                        fprintf( c_file, "%s %s\n",
                                        config.dency_target,
                                        config.dency_added );
                        fclose( c_file );
                }

        } else {
                /* Create new configuration file */
                c_file = fopen( config.dency_config, "w" );
                fprintf( c_file, "%s %s\n",
                                config.dency_target,
                                config.dency_added );
                fclose( c_file );
        }

        return;
}
/* Macro for adding SA-MP plugins */
#define S_ADD_PLUGIN( x, y, z ) \
        dency_implementation_samp_conf( ( dencyconfig ){ x, y, z } )

/* 
 * Add plugin dependency to open.mp JSON configuration
 * Updates legacy_plugins array in server configuration
 */
void dency_implementation_omp_conf( const char* config_name, const char* package_name ) {
        if ( wg_server_env() != 2 )
                return;

        pr_color( stdout, FCOLOUR_GREEN,
                "Create Dependencies '%s' into '%s'\t\t[All good]\n",
                package_name, config_name );

        FILE* c_file = fopen( config_name, "r" );
        cJSON* s_root = NULL;

        /* Load or create JSON configuration */
        if ( !c_file ) {
                s_root = cJSON_CreateObject();
        } else {
                fseek( c_file, 0, SEEK_END );
                long file_size = ftell( c_file );
                fseek( c_file, 0, SEEK_SET );

                char* buffer = ( char* )wg_malloc( file_size + 1 );
                if ( !buffer ) {
                        pr_error( stdout, "Memory allocation failed!" );
                        fclose( c_file );
                        return;
                }

                size_t file_read = fread( buffer, 1, file_size, c_file );
                if ( file_read != file_size ) {
                        pr_error( stdout, "Failed to read the entire file!" );
                        wg_free( buffer );
                        fclose( c_file );
                        return;
                }

                buffer[file_size] = '\0';
                fclose( c_file );

                s_root = cJSON_Parse( buffer );
                wg_free( buffer );

                if ( !s_root ) {
                        s_root = cJSON_CreateObject();
                }
        }

        /* Navigate to pawn -> legacy_plugins structure */
        cJSON* pawn = cJSON_GetObjectItem( s_root, "pawn" );
        if ( !pawn ) {
                pawn = cJSON_CreateObject();
                cJSON_AddItemToObject( s_root, "pawn", pawn );
        }

        cJSON* legacy_plugins;
        legacy_plugins = cJSON_GetObjectItem( pawn, "legacy_plugins" );
        if ( !legacy_plugins ) {
                legacy_plugins = cJSON_CreateArray();
                cJSON_AddItemToObject( pawn,
                                "legacy_plugins",
                                legacy_plugins );
        }

        /* Ensure legacy_plugins is an array */
        if ( !cJSON_IsArray( legacy_plugins ) ) {
                cJSON_DeleteItemFromObject( pawn, "legacy_plugins" );
                legacy_plugins = cJSON_CreateArray();
                cJSON_AddItemToObject( pawn,
                                "legacy_plugins",
                                legacy_plugins );
        }

        /* Check if plugin already exists */
        cJSON* dir_item;
        int p_exist = 0;

        cJSON_ArrayForEach( dir_item, legacy_plugins ) {
                if ( cJSON_IsString( dir_item ) &&
                        !strcmp( dir_item->valuestring, package_name ) ) {
                        p_exist = 1;
                        break;
                }
        }

        /* Add plugin if not already present */
        if ( !p_exist ) {
                cJSON *size_package_name = cJSON_CreateString( package_name );
                cJSON_AddItemToArray( legacy_plugins, size_package_name );
        }

        /* Write updated configuration */
        char* __cJSON_Printed = cJSON_Print( s_root );
        c_file = fopen( config_name, "w" );
        if ( c_file ) {
                fputs( __cJSON_Printed, c_file );
                fclose( c_file );
        }

        /* Cleanup */
        cJSON_Delete( s_root );
        wg_free( __cJSON_Printed );

        return;
}
/* Macro for adding open.mp plugins */
#define M_ADD_PLUGIN( x, y ) dency_implementation_omp_conf( x, y )

/* 
 * Add include directive to gamemode file
 * Inserts #include statement in appropriate location
 */
void dency_add_include( const char *modes, char *dency_name, char *dency_after ) {
        if ( path_exists( modes ) == 0 ) {
                pr_color( stdout, FCOLOUR_GREEN,
                        "~ can't found %s.. skipping adding include name\t\t[Fail]\n", modes );
                return;
        }
        pr_color( stdout, FCOLOUR_GREEN,
                "Create include '%s' into '%s' after '%s'\t\t[All good]\n",
                dency_name, modes, dency_after );

        FILE *m_file = fopen( modes, "r" );
        if ( m_file == NULL ) {
                return;
        }

        /* Read entire file */
        fseek( m_file, 0, SEEK_END );
        long fle_size = ftell( m_file );
        fseek( m_file, 0, SEEK_SET );

        char *ct_modes = wg_malloc( fle_size + 1 );
        size_t file_read = fread( ct_modes, 1, fle_size, m_file );
        if ( file_read != fle_size ) {
                wg_free( ct_modes );
                fclose( m_file );
                return;
        }

        ct_modes[fle_size] = '\0';
        fclose( m_file );

        /* Check if include already exists */
        if ( strstr( ct_modes, dency_name ) != NULL &&
                strstr( ct_modes, dency_after ) != NULL ) {
                wg_free( ct_modes );
                return;
        }

        /* Find position to insert include */
        char *e_dency_after_pos = strstr( ct_modes, dency_after );

        FILE *n_file = fopen( modes, "w" );
        if ( n_file == NULL ) {
                wg_free( ct_modes );
                return;
        }

        if ( e_dency_after_pos != NULL ) {
                /* Insert after specified include */
                fwrite( ct_modes, 1,
                           e_dency_after_pos - ct_modes + strlen( dency_after ),
                           n_file );
                fprintf( n_file, "\n%s", dency_name );
                fputs( e_dency_after_pos + strlen( dency_after ), n_file );
        } else {
                /* Alternative insertion strategy */
                char *include_to_add_pos = strstr( ct_modes, dency_name );

                if ( include_to_add_pos != NULL ) {
                        fwrite( ct_modes, 1,
                                   include_to_add_pos - ct_modes + strlen( dency_name ),
                                   n_file );
                        fprintf( n_file, "\n%s", dency_after );
                        fputs( include_to_add_pos + strlen( dency_name ), n_file );
                } else {
                        /* Prepend at beginning */
                        fprintf( n_file, "%s\n%s\n%s", dency_after, dency_name, ct_modes );
                }
        }

        fclose( n_file );
        wg_free( ct_modes );

        return;
}
/* Macro for adding include directives */
#define DENCY_ADD_INCLUDES( x, y, z ) dency_add_include( x, y, z )

/* 
 * Process and add include directive based on dependency
 * Reads TOML configuration to determine gamemode file
 */
static void dency_include_prints( const char *package_include )
{
        char wg_buf_err[WG_PATH_MAX];
        toml_table_t *wg_toml_config;
        char depends_name[WG_PATH_MAX];
        char idirective[WG_MAX_PATH];

        /* Extract filename from path */
        const char *dency_n = dency_get_filename( package_include );
        snprintf( depends_name, sizeof( depends_name ), "%s", dency_n );

        const char *direct_bnames = dency_get_basename( depends_name );

        /* Parse TOML configuration */
        FILE *this_proc_file = fopen( "watchdogs.toml", "r" );
        wg_toml_config = toml_parse_file( this_proc_file, wg_buf_err, sizeof( wg_buf_err ) );
        if ( this_proc_file ) fclose( this_proc_file );

        if ( !wg_toml_config ) {
                pr_error( stdout, "parsing TOML: %s", wg_buf_err );
                return;
        }

        /* Extract gamemode input file from TOML */
        toml_table_t *wg_compiler = toml_table_in( wg_toml_config, "compiler" );
        if ( wg_compiler ) {
                toml_datum_t toml_proj_i = toml_string_in( wg_compiler, "input" );
                if ( toml_proj_i.ok ) {
                        wgconfig.wg_toml_proj_input = strdup( toml_proj_i.u.s );
                        wg_free( toml_proj_i.u.s );
                }
        }
        toml_free( wg_toml_config );

        /* Construct include directive */
        snprintf( idirective, sizeof( idirective ),
                "#include <%s>",
                direct_bnames );

        /* Add include based on server environment */
        if ( wg_server_env() == 1 ) {
                /* SA-MP: include after a_samp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                idirective,
                                "#include <a_samp>" );
        } else if ( wg_server_env() == 2 ) {
                /* open.mp: include after open.mp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                idirective,
                                "#include <open.mp>" );
        } else {
                /* Default: include after a_samp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                idirective,
                                "#include <a_samp>" );
        }
}

/* 
 * Search for and move files matching pattern
 * Handles file relocation and configuration updates
 */
void dump_file_type( const char *dump_path, char *dump_pattern,
                    char *dump_exclude, char *dump_pwd, char *dump_place, int dump_root )
{
        wg_sef_fdir_memset_to_null();

        const char *package_names, *basename;
        char *basename_lower;
        
        /* Search for files matching dump_pattern */
        int i;
        int found = -1;
        found = wg_sef_fdir( dump_path, dump_pattern, dump_exclude );
        ++found_cnt;
#if defined(_DBG_PRINT)
        println(stdout, "found (%d): %d", found_cnt, found);
#endif
        if ( found ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        package_names = dency_get_filename( wgconfig.wg_sef_found_list[i] );
                        basename = dency_get_basename( wgconfig.wg_sef_found_list[i] );
                        
                        /* Convert to lowercase for case-insensitive comparison */
                        basename_lower = strdup( basename );
                        for ( int j = 0; basename_lower[j]; j++ )
                                {
                                        basename_lower[j] = tolower( basename_lower[j] );
                                }
                        
                        /* Prefix checking */
                        int rate_has_prefix = 0;
                        for ( size_t i = 0;
                             i < sizeof( root_keywords ) / sizeof( root_keywords [ 0 ] );
                             i++ )
                        {
                                const char* keyword = root_keywords[ i ];
                                size_t keyword_len = strlen( keyword );
                                if ( strncmp( basename_lower,
                                        keyword,
                                        keyword_len
                                ) == 0 ) {
                                        ++rate_has_prefix;
                                        break;
                                }
                        }

                        /* Move to target directory if specified */
                        if ( dump_place[0] != '\0' ) {
                        #ifdef WG_WINDOWS
                                snprintf( package_command, sizeof( package_command ),
                                        "move "
                                        "/Y \"%s\" \"%s\\%s\\\"",
                                        wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );

                                wg_run_command( package_command );
                        #else
                                snprintf( package_command, sizeof( package_command ),
                                        "mv "
                                        "-f \"%s\" \"%s/%s/\"",
                                        wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );

                                wg_run_command( package_command );
                        #endif
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s - %s\n",
                                                 wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );
                        } else {/* Move to current directory */
                                if ( rate_has_prefix ) {
                                #ifdef WG_WINDOWS
                                        snprintf( package_command, sizeof( package_command ),
                                                "move "
                                                "/Y \"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], dump_pwd );

                                        wg_run_command( package_command );
                                #else
                                        snprintf( package_command, sizeof( package_command ),
                                                "mv "
                                                "-f \"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], dump_pwd );

                                        wg_run_command( package_command );
                                #endif
                                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\n",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                } else {
                                        if ( path_exists( "plugins" ) == 1 ) {
                                        #ifdef WG_WINDOWS
                                                snprintf( package_command, sizeof( package_command ),
                                                        "move "
                                                        "/Y \"%s\" \"%s\\plugins\"",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );

                                                wg_run_command( package_command );

                                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\\plugins\n",
                                                                wgconfig.wg_sef_found_list[i], dump_pwd );
                                        #else
                                                snprintf( package_command, sizeof( package_command ),
                                                        "mv "
                                                        "-f \"%s\" \"%s/plugins\"",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                                
                                                wg_run_command( package_command );

                                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s/plugins\n",
                                                                wgconfig.wg_sef_found_list[i], dump_pwd );
                                        #endif
                                        }
                                }

                                if ( rate_has_prefix ) {
                                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\n",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                } else {
                                        if ( path_exists( "plugins" ) == 1 ) {
                                        #ifdef WG_WINDOWS
                                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\\plugins\n",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                        #else
                                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s/plugins\n",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                        #endif
                                        }
                                }
                                snprintf( json_item, sizeof( json_item ), "%s", package_names );
                                dency_set_hash( json_item, json_item );

                                if ( dump_root == 1 ) {
                                        goto done;
                                }

                                /* Update configuration based on server environment */
                                if ( wg_server_env() == 1 && strfind( wgconfig.wg_toml_config, ".cfg", true ) )
                                        S_ADD_PLUGIN( wgconfig.wg_toml_config, "plugins", basename );
                                else if ( wg_server_env() == 2 && strfind( wgconfig.wg_toml_config, ".json", true ) )
                                        M_ADD_PLUGIN( wgconfig.wg_toml_config, basename );
                        }
                }
        }
done:
        return;
}

/* 
 * Utility function to add items between cJSON arrays
 * Copies string items from one array to another
 */
void dency_cjson_additem( cJSON *p1, int p2, cJSON *p3 )
{
        if ( cJSON_IsString( cJSON_GetArrayItem( p1, p2 ) ) )
                cJSON_AddItemToArray( p3,
                        cJSON_CreateString( cJSON_GetArrayItem( p1, p2 )->valuestring ) );
}

/* 
 * Organize dependency files into appropriate directories
 * Moves plugins, includes, and components to their proper locations
 */
void dency_move_files( const char *dency_dir )
{
        /* Char */
        char root_dir[WG_PATH_MAX], include_path[WG_MAX_PATH],
             plug_path[WG_PATH_MAX], comp_path[WG_PATH_MAX],
             install_path[WG_PATH_MAX * 2],
             full_path[WG_PATH_MAX],
             src[WG_PATH_MAX],
             dest[WG_MAX_PATH * 2];

        /* Pointer */
        const char *procure_depends_name;
        char *package_include_path = NULL;
        char *filename = NULL;

        /* Int */
        int i;
        int index = -1;
        int stack_size = WG_MAX_PATH;
        int package_inc = 0;
        
        /* Reset */
        found_cnt = 0;

        /* CWD/PWD */
        char *cwd = wg_procure_pwd();

        /* Stack-based directory traversal for finding nested include files */
        struct stat dir_st;
        struct dirent *dir_item;
        char **tmp_stack = wg_malloc( stack_size * sizeof( char* ) );
        if ( tmp_stack == 0 )
                {
                        return;
                }
        char **stack = tmp_stack;

        for ( i = 0; i < stack_size; i++ )
                {
                        stack[i] = wg_malloc( WG_PATH_MAX );
                }
        
        /* Construct platform-specific paths */
        #ifdef WG_WINDOWS
        snprintf( plug_path, sizeof( plug_path ), "%s\\plugins",
                dency_dir );
        snprintf( comp_path, sizeof( comp_path ), "%s\\components",
                dency_dir );
        #else
        snprintf( plug_path, sizeof( plug_path ), "%s/plugins",
                dency_dir );
        snprintf( comp_path, sizeof( comp_path ), "%s/components",
                dency_dir );
        #endif

#if defined(_DBG_PRINT)
        println(stdout, "plug path: %s", plug_path);
#endif

#if defined(_DBG_PRINT)
        println(stdout, "comp path: %s", comp_path);
#endif

        /* Determine include path based on server environment */
        if ( wg_server_env() == 1 ) {
        #ifdef WG_WINDOWS
                package_include_path = "pawno\\include";
                snprintf( full_path, sizeof( full_path ), "%s\\pawno\\include", dency_dir );
        #else
                package_include_path = "pawno/include";
                snprintf( full_path,  sizeof( full_path ), "%s/pawno/include", dency_dir );
        #endif
        } else if ( wg_server_env() == 2 ) {
        #ifdef WG_WINDOWS
                package_include_path = "qawno\\include";
                snprintf( full_path, sizeof( full_path ), "%s\\qawno\\include", dency_dir );
        #else
                package_include_path = "qawno/include";
                snprintf( full_path, sizeof( full_path ), "%s/qawno/include", dency_dir );
        #endif
        }

#if defined(_DBG_PRINT)
        println(stdout, "package include path: %s", package_include_path);
#endif

#if defined(_DBG_PRINT)
        println(stdout, "full path: %s", full_path);
#endif

        wg_sef_fdir_memset_to_null();

        /* Move include files from standard include directory */
        package_inc = wg_sef_fdir( full_path, "*.inc", NULL );
#if defined(_DBG_PRINT)
        println(stdout, "package inc: %d", package_inc);
#endif
        if ( package_inc ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        procure_depends_name = dency_get_filename( wgconfig.wg_sef_found_list[i] );
                #ifdef WG_WINDOWS
                        snprintf( package_command, sizeof( package_command ),
                                "move "
                                "/Y \"%s\" \"%s\\%s\\\"",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );

                        wg_run_command( package_command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );
                #else
                        snprintf( package_command, sizeof( package_command ),
                                "mv "
                                "-f \"%s\" \"%s/%s/\"",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );

                        wg_run_command( package_command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );
                #endif

                        dency_set_hash( procure_depends_name,
                                procure_depends_name );
                        dency_include_prints( procure_depends_name );
                }
        }

        wg_sef_fdir_memset_to_null();

        /* Move include files from root of dency_dir */
        package_inc = wg_sef_fdir( dency_dir, "*.inc", NULL );
#if defined(_DBG_PRINT)
        println(stdout, "package inc2: %d", package_inc);
#endif
        if ( package_inc ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        const char *procure_depends_name;
                        procure_depends_name = dency_get_filename( wgconfig.wg_sef_found_list[i] );

                #ifdef WG_WINDOWS
                        snprintf( package_command, sizeof( package_command ),
                                "move "
                                "/Y \"%s\" \"%s\\%s\\\"",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );

                        wg_run_command( package_command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );
                #else
                        snprintf( package_command, sizeof( package_command ),
                                "mv "
                                "-f \"%s\" \"%s/%s/\"",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );

                        wg_run_command( package_command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
                                wgconfig.wg_sef_found_list[i], cwd, package_include_path );
                #endif

                        dency_set_hash( procure_depends_name,
                                procure_depends_name );
                        dency_include_prints( procure_depends_name );
                }
        }

        /* Normalize path separators */
        char *path_pos;
        while ( ( path_pos = strstr( package_include_path, "include\\" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include\\" ) ) + 1 );
        while ( ( path_pos = strstr( package_include_path, "include/" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include/" ) ) + 1 );
        while ( ( path_pos = strstr( full_path, "include\\" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include\\" ) ) + 1 );
        while ( ( path_pos = strstr( full_path, "include/" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include/" ) ) + 1 );

        /* Move plugin files */
#ifndef WG_WINDOWS
        dump_file_type( plug_path, "*.dll", NULL, cwd, "/plugins", 0 );
        dump_file_type( plug_path, "*.so", NULL, cwd, "/plugins", 0 );
#else
        dump_file_type( plug_path, "*.dll", NULL, cwd, "\\plugins", 0 );
        dump_file_type( plug_path, "*.so", NULL, cwd, "\\plugins", 0 );
#endif

#if defined(_DBG_PRINT)
        println(stdout, "plug path: %s", plug_path);
#endif
        /* Move root-level plugin files */
        dump_file_type( dency_dir, "*.dll", "plugins", cwd, "", 1 );
        dump_file_type( dency_dir, "*.so", "plugins", cwd, "", 1 );

        /* Handle open.mp specific components */
        if ( wg_server_env() == 2 ) {
        #ifdef WG_WINDOWS
                snprintf( include_path, sizeof( include_path ), "qawno\\include" );
        #else
                snprintf( include_path, sizeof( include_path ), "qawno/include" );
        #endif
                dump_file_type( comp_path, "*.dll", NULL, cwd, "components", 0 );
                dump_file_type( comp_path, "*.so", NULL, cwd, "components", 0 );
        } else {
        #ifdef WG_WINDOWS
                snprintf( include_path, sizeof( include_path ), "pawno\\include" );
        #else
                snprintf( include_path, sizeof( include_path ), "pawno/include" );
        #endif
        }

        /* Normalize include path */
        while ( ( path_pos = strstr( include_path, "include\\" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include\\" ) ) + 1 );
        while ( ( path_pos = strstr( include_path, "include/" ) ) != NULL )
                memmove( path_pos, path_pos + strlen( "include/" ),
                        strlen( path_pos + strlen( "include/" ) ) + 1 );

        /* Start with dependency directory */
        ++index;
        snprintf( stack[index], WG_PATH_MAX, "%s", dency_dir );

        /* Depth-first search for include files */
        while ( index >= 0 ) {
                strlcpy( root_dir, stack[index], sizeof( root_dir ) );
                --index;

                DIR *open_dir = opendir( root_dir );
                if ( !open_dir )
                        {
                                continue;
                        }

                while ( ( dir_item = readdir( open_dir ) ) != NULL ) {
                        if ( wg_is_special_dir( dir_item->d_name ) )
                                {
                                        continue;
                                }

                        /* Construct full path */
                        strlcpy( install_path,
                                root_dir, sizeof( install_path ) );
                        strlcat( install_path,
                                __PATH_STR_SEP_LINUX, sizeof( install_path ) );
                        strlcat( install_path,
                                dir_item->d_name, sizeof( install_path ) );

                        if ( stat( install_path, &dir_st ) != 0 )
                                {
                                        continue;
                                }
                                
                        if ( S_ISDIR( dir_st.st_mode ) ) {
                                /* Skip compiler directories */
                                if ( strcmp( dir_item->d_name, "pawno" ) == 0 ||
                                    strcmp( dir_item->d_name, "qawno" ) == 0 ||
                                    strcmp( dir_item->d_name, "include" ) == 0 )
                                {
                                        continue;
                                }
                                /* Push directory onto stack for later processing */
                                if ( index < stack_size - 1 )
                                        {
                                                ++index;
                                                strlcpy( stack[index],
                                                        install_path, WG_MAX_PATH );
                                        }
                                continue;
                        }

                        /* Process ".inc" files */
                        if ( !strrchr( dir_item->d_name, '.' ) ||
                            strcmp( strrchr( dir_item->d_name, '.' ), ".inc" ) == 0 )
                        {
                                continue;
                        }

                        /* Extract parent directory name */
                        strlcpy( src, root_dir, sizeof( src ) );
                        #ifdef WG_WIDOWS
                        filename = strrchr( src, __PATH_CHR_SEP_WIN32 );
                        #else
                        filename = strrchr( src, __PATH_CHR_SEP_LINUX );
                        #endif
                        if ( !filename )
                                {
                                        continue;
                                }
                        ++filename;

                        /* Construct destination path */
                        strlcpy( dest, include_path, sizeof( dest ) );
                        #ifdef WG_WINDOWS
                        strlcat( dest, __PATH_STR_SEP_WIN32, sizeof( dest ) );
                        #else
                        strlcat( dest, __PATH_STR_SEP_LINUX, sizeof( dest ) );
                        #endif
                        strlcat( dest, filename, sizeof( dest ) );

                        /* Move or copy directory containing include files */
                        if ( rename( src, dest ) ) {
                        #ifdef WG_WINDOWS
                                /* Windows: copy directory tree and remove original */
                                const char *windows_move_parts[] = {
                                        "xcopy ", src, " ", dest, " /E /I /H /Y " ">nul 2>&1 && " "rmdir /S /Q ", src, " >nul 2>&1"
                                };
                                size_t size_windows_move_parts = sizeof( windows_move_parts ),
                                       size_windows_move_parts_zero = sizeof( windows_move_parts[0] );
                                strlcpy( package_command, "", sizeof( package_command ) );
                                int j;
                                for ( j = 0;
                                      j < size_windows_move_parts / size_windows_move_parts_zero;
                                      j++ )
                                {
                                        strlcat( package_command, windows_move_parts[j], sizeof( package_command ) );
                                }
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s\\?\n",
                                                 src, dest );
                        #else
                                /* Unix: recursive copy and remove */
                                const char *unix_move_parts[] = {
                                        "cp -r ", src, " ", dest, " && rm -rf ", src
                                };
                                size_t size_unix_move_parts = sizeof( unix_move_parts ),
                                       size_unix_move_parts_zero = sizeof( unix_move_parts[0] );
                                strlcpy( package_command, "", sizeof( package_command ) );
                                int j;
                                for ( j = 0;
                                      j < size_unix_move_parts / size_unix_move_parts_zero;
                                      j++ )
                                {
                                        strlcat( package_command, unix_move_parts[j], sizeof( package_command ) );
                                }
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s/?\n",
                                                 src, dest );
                        #endif
                        }

                        dency_set_hash( dest, dest );
                }

                closedir( open_dir );
        }

        /* Clean up directory stack */
        for ( i = 0;
              i < stack_size;
              i++ ) {
                wg_free( stack [ i ] );
        }
        wg_free( stack );

        printf( "\n" );

        /* Remove extracted dependency directory */
        #ifdef WG_WINDOWS
        snprintf( package_command, sizeof( package_command ),
                "rmdir "
                "/s "
                "/q \"%s\"",
                dency_dir );
        #else
        snprintf( package_command, sizeof( package_command ),
                "rm "
                "-rf %s",
                dency_dir );
        #endif
        wg_run_command( package_command );

        return;
}

/* 
 * Prepare and organize dependencies after download
 * Creates necessary directories and initiates file organization
 */
void wg_apply_depends( const char *depends_name )
{
        char _dependencies[WG_PATH_MAX];
        char dency_dir[WG_PATH_MAX];
        
        snprintf( _dependencies, sizeof( _dependencies ), "%s", depends_name );

#if defined(_DBG_PRINT)
        println(stdout, "_dependencies: %s", _dependencies);
#endif

        /* Remove archive extension to get directory name */
        char *extension = NULL;
        if ((extension = strstr(_dependencies, ".tar.gz")) != NULL) {
                *extension = '\0';
        } else if ((extension = strstr(_dependencies, ".tar")) != NULL) {
                *extension = '\0';
        } else if ((extension = strstr(_dependencies, ".zip")) != NULL) {
                *extension = '\0';
        }

        snprintf( dency_dir, sizeof( dency_dir ), "%s", _dependencies );

#if defined(_DBG_PRINT)
        println(stdout, "dency dir: %s", dency_dir);
#endif

        /* Create necessary directories based on server environment */
        if ( wg_server_env() == 1 ) {
                if ( dir_exists( "pawno/include" ) == 0 ) {
                        wg_mkdir( "pawno/include" );
                }
                if ( dir_exists( "plugins" ) == 0 ) {
                        MKDIR( "plugins" );
                }
        } else if ( wg_server_env() == 2 ) {
                if ( dir_exists( "qawno/include" ) == 0 ) {
                        wg_mkdir( "qawno/include" );
                }
                if ( dir_exists( "plugins" ) == 0 ) {
                        MKDIR( "plugins" );
                }
                if ( dir_exists( "components" ) == 0 ) {
                        MKDIR( "components" );
                }
        }
        
        /* Organize dependency files */
        dency_move_files( dency_dir );
}

/* 
 * Main dependency installation function
 * Parses dependency strings, downloads archives, and applies dependencies
 */
void wg_install_depends( const char *dependencies_str, const char *dependencies_branch )
{
        char buffer[1024];
        char dency_url[1024];
        char *procure_buffer;
        struct dency_repositories repo;
        char dency_name[WG_PATH_MAX];
        const char *size_last_slash;
        const char *dependencies[MAX_DEPENDS];
        int package_counts = 0;
        int i;

        memset( dependencies, 0, sizeof( dependencies ) );
        wgconfig.wg_idepends = 0;

        /* Validate input */
        if ( !dependencies_str || !*dependencies_str )
                {
                        pr_color( stdout, FCOLOUR_RED, "" );
                        printf( "no valid dependencies to install!"
                                "\t\t[Fail]\n" );
                        goto done;
                }

        if ( strlen( dependencies_str ) >= sizeof( buffer ) )
                {
                        pr_color( stdout, FCOLOUR_RED, "" );
                        printf( "dependencies_str too long!"
                                "\t\t[Fail]\n" );
                        goto done;
                }

        /* Tokenize dependency string */
        snprintf( buffer, sizeof( buffer ), "%s", dependencies_str );

        procure_buffer = strtok( buffer, " " );
        while ( procure_buffer
               && package_counts < MAX_DEPENDS )
        {
                dependencies[package_counts++] = procure_buffer;
                procure_buffer = strtok( NULL, " " );
        }

        if ( !package_counts )
                {
                        pr_color( stdout, FCOLOUR_RED, "" );
                        printf( "no valid dependencies to install!"
                                "\t\t[Fail]\n" );
                        goto done;
                }

        /* Process each dependency */
        for ( i = 0; i < package_counts; i++ ) {
                /* Parse repository information */
                if ( !dency_parse_repo( dependencies[i], &repo ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "invalid repo format: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                
                /* Handle GitHub repositories */
                if ( !strcmp( repo.host, "github" ) ) {
                        if ( !dency_handle_repo( &repo, dency_url,
                            sizeof( dency_url ),
                            dependencies_branch ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "repo not found: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                } else {
                        /* Handle custom repositories */
                        dency_build_repo_url( &repo, 0, dency_url, sizeof( dency_url ) );
                        if ( !dency_url_checking( dency_url,
                            wgconfig.wg_toml_github_tokens ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "repo not found: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                }

                /* Extract filename from URL */
                if ( strrchr( dency_url, __PATH_CHR_SEP_LINUX ) &&
                    *( strrchr( dency_url, __PATH_CHR_SEP_LINUX ) + 1 ) )
                {
                        snprintf( dency_name, sizeof( dency_name ), "%s",
                                strrchr( dency_url, __PATH_CHR_SEP_LINUX ) + 1 );

                        /* Ensure archive extension */
                        if ( !strend( dency_name, ".tar.gz", true ) &&
                            !strend( dency_name, ".tar", true ) &&
                            !strend( dency_name, ".zip", true ) )
                        {
                                snprintf( dency_name + strlen( dency_name ),
                                        sizeof( dency_name ) - strlen( dency_name ), ".zip" );
                        }
                } else {
                        /* Default naming scheme */
                        snprintf( dency_name, sizeof( dency_name ), "%s.tar.gz", repo.repo );
                }

                if ( !*dency_name ) {
                        pr_color( stdout, FCOLOUR_RED, "" );
                        printf( "invalid repo name: %s\t\t[Fail]\n", dency_url );
                        continue;
                }

                wgconfig.wg_idepends = 1;

                struct timespec __time_start = { 0 },   /* Time start - duration calculation */
                                __time_stop  = { 0 };   /* Time Stop   - duration calculation */
                double dency_dur_calculation;           /* Time Start & Stop Calculation */

                /* Time the dependency installation process */
                wg_download_file( dency_url, dency_name );
                clock_gettime( CLOCK_MONOTONIC, &__time_start );
                        wg_apply_depends( dency_name );
                clock_gettime( CLOCK_MONOTONIC, &__time_stop );

                dency_dur_calculation = ( __time_stop.tv_sec - __time_start.tv_sec   ) +
                                        ( __time_stop.tv_nsec - __time_start.tv_nsec ) / 1e9;
                
                pr_color( stdout, FCOLOUR_CYAN, " <D> Finished at %.3fs (%.0f ms)\n",
                        dency_dur_calculation, dency_dur_calculation * 1000.0 );
        }

done:
        return;
}
