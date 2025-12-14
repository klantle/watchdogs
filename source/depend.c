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
#include "debug.h"
#include "depend.h"

/* Global buffers for dependency management operations */
static char command[ WG_MAX_PATH * 4 ];    /* Buffer for system commands */
static char *tag;    /* Version tag for dependencies */
static char *path;    /* Path component of dependency URL */
static char *path_slash;    /* Pointer to path separator */
static char *user;    /* Repository owner/username */
static char *repo;    /* Repository name */
static char *repo_slash;    /* Pointer to repository separator */
static char *git_dir;    /* Pointer to .git extension */
static char *filename;    /* Extracted filename */
static char *extension;    /* File extension */
static char json_item[WG_PATH_MAX];    /* Json item's */
static int fdir_counts = 0;    /* dump file counts */

/*
 * This is the maximum dependency in 'watchdogs.toml' that can be read from the 'packages' key,
 * and you need to ensure this capacity is more than sufficient. If you enter 101, it will only read up to 100.
*/
#define MAX_DEPENDS (102)

struct repositories {
        char host[32]  ; /* repo host */
        char domain[64]; /* repo domain */
        char user[128] ; /* repo user */
        char repo[128] ; /* repo name */
        char tag[128]  ; /* repo tag */
};

const char *matching_windows_patterns[] = {
        "windows",
        "win",
        "win32",
        "win32",
        "msvc",
        "mingw",
        ".dll",
        NULL
};
    
const char *matching_linux_patterns[] = {
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
        "dependencies",
        NULL
};

/* Path Separator */
static inline const char
        * PATH_SEPARATOR( const char *path )
{
        const char
                *l = strrchr( path, __PATH_CHR_SEP_LINUX ),
                *w = strrchr( path, __PATH_CHR_SEP_WIN32 );
        if ( l && w )
                return ( l > w ) ? l : w;
        return l ? l : w;
}

/* 
 * Convert Windows path separators to Unix/Linux style
 * Replaces backslashes with forward slashes for cross-platform compatibility
 */
void package_sym_convert( char *path )
{
        char *pos;
        for ( pos = path; *pos; pos++ ) if ( *pos == __PATH_CHR_SEP_WIN32 )
        {
                *pos = __PATH_CHR_SEP_LINUX;
        }
}

/* 
 * Extract filename from full path (excluding directory)
 * Returns pointer to filename component
 */
const char *package_get_filename(
        const char *path
) {
        const char *packages = PATH_SEPARATOR( path );
        return packages ?
               packages + 1 : path;
}

/* 
 * Extract basename from full path (similar to package_get_filename)
 * Alternative implementation for filename extraction
 */
static const char *package_get_basename(
        const char *path
) {
        const char *p = PATH_SEPARATOR( path );
        return p ?
               p + 1 : path;
}

/*
 * this_os_archive - check if filename indicates OS-specific archive
 *
 * Identifies platform-specific files by name patterns. Returns 1 if the
 * filename contains any OS pattern, 0 otherwise.
 */
int this_os_archive( const char *filename ) {
        int k;
        
        /* Determine target operating system at compile time */
        const char *target_os = NULL;
        #ifdef WG_WINDOWS
        target_os = "windows";
        #else
        target_os = "linux";
        #endif
        
        if ( !target_os ) return 0;
        
        /* Convert target_os to lowercase for case-insensitive comparison */
        char size_target[ WG_PATH_MAX ];
        strncpy( size_target, target_os, sizeof ( size_target ) - 1 );
        size_target[ sizeof( size_target ) - 1 ] = '\0';
        
        for (int i = 0; size_target[ i ]; i++) {
            size_target[ i ] = tolower(size_target[ i ]);
        }
        
        /* Select OS-specific patterns */
        const char **patterns = NULL;
        if (strstr( size_target, "win" )) {
            patterns = matching_windows_patterns;
        } else if (strstr( size_target, "linux")) {
            patterns = matching_linux_patterns;
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
                return 1;
            }
        }
        
        return 0;
}

/*
 * this_more_archive - check if filename indicates from "matching_root_keywords" archive
 *
 * Identifies server-related files by name patterns. Returns 1 if the
 * filename contains any server pattern, 0 otherwise.
 */
int this_more_archive(const char *filename)
{
        int k;                     /* loop counter for array _sindex */
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
 * package_fetching_assets - select most appropriate asset from available options
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
char *package_fetching_assets(char **package_assets,
                            int counts, const char *pf_os)
{
        int i;
        int rate_os_pattern = 0;

        /* Handle edge cases */
        if (counts == 0)
            return NULL;
        if (counts == 1)
            return strdup(package_assets[ 0 ]);
        
        const char *target_os = NULL;
        if (pf_os && pf_os[ 0 ]) {
            target_os = pf_os;
        } else {
            #ifdef WG_WINDOWS
            target_os = "windows";
            #else
            target_os = "linux";
            #endif
        }
        
        char size_target[ 32 ] = { 0 };
        if (target_os) {
            strncpy( size_target,
                     target_os,
                     sizeof(size_target) - 1 );
            for (int j = 0; size_target[j]; j++) {
                size_target[j] = tolower(size_target[j]);
            }
        }

        if (size_target[ 0 ]) {
            for (i = 0; i < counts; i++) {
                int is_server_asset = 0;
                for (int p = 0;
                     matching_any_patterns[ p ] != NULL;
                     p++) {
                    if (strfind(package_assets[ i ],
                        matching_any_patterns[ p ], true)) {
                        is_server_asset = 1;
                        break;
                    }
                }
                
                if (is_server_asset) {
                    if (strstr(size_target, "win")) {
                        for (int w = 0; matching_windows_patterns[ w ] != NULL; w++)
                        {
                            if ( strstr(package_assets[ i ],
                                matching_windows_patterns[ w ] )
                            ) {
                                return
                                        strdup( package_assets [ i ] );
                            }
                        }
                    } else if (strstr(size_target, "linux")) {
                        for (int l = 0; matching_linux_patterns[ l ] != NULL; l++)
                        {
                            if (strstr( package_assets[ i ],
                                matching_linux_patterns[ l ])
                            ) {
                                return strdup( package_assets [ i ] );
                            }
                        }
                    }
                }
            }
        }
        
        if (size_target[ 0 ]) {
            for (i = 0; i < counts; i++) {
                if (strstr( size_target, "win") ) {
                    for (int w = 0; matching_windows_patterns[ w ] != NULL; w++)
                    {
                        if (strstr(package_assets[ i ],
                            matching_windows_patterns[ w ]))
                        {
                            return
                                strdup( package_assets [ i ] );
                        }
                    }
                } else if ( strstr(size_target, "linux") ) {
                    for (int l = 0; matching_linux_patterns[ l ] != NULL; l++)
                    {
                        if (strstr(package_assets[ i ],
                            matching_linux_patterns[ l ])) {
                            return
                                strdup( package_assets [ i ] );
                        }
                    }
                }
            }
        }
        
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
        
        for (i = 0; i < counts; i++) {
            for (int w = 0;
                 matching_windows_patterns[ w ] != NULL &&
                 !rate_os_pattern; w++)
            {
                if ( strstr(package_assets[ i ],
                    matching_windows_patterns[ w ]) )
                {
                    rate_os_pattern = 1;
                }
            }
            for (int l = 0;
                 matching_linux_patterns[ l ] != NULL &&
                 ! rate_os_pattern; l++)
            {
                if ( strstr(package_assets[ i ],
                    matching_linux_patterns[ l ]) )
                {
                    rate_os_pattern = 1;
                }
            }
            
            if ( ! rate_os_pattern ) {
                return
                        strdup( package_assets [ i ] );
            }
        }
        
        return
                strdup( package_assets [ 0 ] );
}

/* 
 * Parse repository URL/identifier into structured components
 * Extracts host, domain, user, repo, and tag from dependency string
 */
static int
package_parse_repo( const char *input, struct repositories *__package_data ) {

        memset( __package_data, 0, sizeof( *__package_data ) );

        /* Default to GitHub */
        strcpy( __package_data->host, "github" );
        strcpy( __package_data->domain, "github.com" );

        char parse_input[WG_PATH_MAX * 2];
        strncpy( parse_input, input, sizeof( parse_input ) - 1 );
        parse_input[sizeof( parse_input ) - 1] = '\0';

        /* Extract tag (version) if present (format: user/repo?tag) */
        tag = strrchr( parse_input, '?' );
        if ( tag ) {
                *tag = '\0';
                strncpy( __package_data->tag,
                        tag + 1, sizeof( __package_data->tag ) - 1 );

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
        if ( git_dir ) { *git_dir = '\0'; }
        strncpy( __package_data->repo,
                repo, sizeof( __package_data->repo ) - 1 );

        if ( strlen( __package_data->user ) == 0 ||
             strlen( __package_data->repo ) == 0 )
                return 0;

#if defined( _DBG_PRINT )
        char size_title[WG_PATH_MAX * 3];
        snprintf( size_title, sizeof( size_title ),
                "repo: host=%s, domain=%s, user=%s, repo=%s, tag=%s",
                __package_data->host, __package_data->domain,
                __package_data->user, __package_data->repo,
                __package_data->tag[0] ? __package_data->tag : "(none)" );
        wg_console_title( size_title );
#endif

        return 1;
}

/* 
 * Fetch release asset URLs from GitHub repository
 * Retrieves downloadable assets for a specific release tag
 */
static int package_gh_release_assets( const char *user, const char *repo,
                                 const char *tag, char **out_urls, int max_urls ) {
                                        
        char api_url[WG_PATH_MAX * 2];
        char *json_data = NULL;
        const char *p;
        int url_count = 0;

        /* Build GitHub API URL for specific release tag */
        snprintf( api_url, sizeof( api_url ),
                         "%sapi.github.com/repos/%s/%s/releases/tags/%s",
                         "https://", user, repo, tag );

        /* Fetch release information JSON */
        if ( !package_http_get_content( api_url, wgconfig.wg_toml_github_tokens, &json_data ) )
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
package_build_repo_url( const struct repositories *__package_data, int is_tag_page,
                   char *put_url, size_t put_size )
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
                                snprintf( put_url, put_size,
                                        "https://%s/%s/%s/releases/latest",
                                        __package_data->domain, __package_data->user,
                                        __package_data->repo );
                        } else {
                                snprintf( put_url, put_size,
                                        "https://%s/%s/%s/releases/tag/%s",
                                        __package_data->domain, __package_data->user,
                                        __package_data->repo, package_actual_tag );
                        }
                } else if ( package_actual_tag[0] ) {
                        /* Direct archive URL for specific tag */
                        if ( !strcmp( package_actual_tag, "latest" ) ) {
                                snprintf( put_url, put_size,
                                        "https://%s/%s/%s/releases/latest",
                                        __package_data->domain, __package_data->user,
                                        __package_data->repo );
                        } else {
                                snprintf( put_url, put_size,
                                        "https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
                                        __package_data->domain, __package_data->user,
                                        __package_data->repo, package_actual_tag );
                        }
                } else {
                        /* Default to main branch ZIP */
                        snprintf( put_url, put_size,
                                "https://%s/%s/%s/archive/refs/heads/main.zip",
                                __package_data->domain, __package_data->user,
                                __package_data->repo );
                }
        }

#if defined( _DBG_PRINT )
        pr_info( stdout, "Built URL: %s (is_tag_page=%d, tag=%s)",
                put_url, is_tag_page,
                package_actual_tag[0] ? package_actual_tag : "(none)" );
#endif
}

/* 
 * Fetch latest release tag from GitHub repository
 * Uses GitHub API to get the most recent release tag
 */
static int package_gh_latest_tag( const char *user, const char *repo,
                             char *out_tag, size_t put_size )
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
        if ( !package_http_get_content( api_url, wgconfig.wg_toml_github_tokens, &json_data ) )
                return 0;

        /* Parse JSON to extract tag_name field */
        p = strstr( json_data, "\"tag_name\"" );
        if ( p ) {
                p = strchr( p, ':' );
                if ( p ) {
                        ++p; /* skip colon */
                        /* Skip whitespace */
                        while ( *p &&
                                   ( *p == ' ' ||
                                   *p == '\t' ||
                                   *p == '\n' ||
                                   *p == '\r' )
                                  )
                                  ++p;

                        if ( *p == '"' ) {
                                ++p; /* skip opening quote */
                                const char *end = strchr( p, '"' );
                                if ( end ) {
                                        size_t tag_len = end - p;
                                        if ( tag_len < put_size ) {
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
static int
package_handle_repo( const struct repositories *repo, char *put_url, size_t put_size, const char *branch ) {

        int ret = 0;
         /* Common branch name */
        const char *package_repo_branch[] = { branch, "main", "master" };
        char package_actual_tag[128] = { 0 };
        int use_fallback_branch = 0;

        /* Handle "latest" tag by resolving to actual tag name */
        if ( repo->tag[0] && strcmp( repo->tag, "newer" ) == 0 ) {
            if ( package_gh_latest_tag( repo->user,
                    repo->repo,
                    package_actual_tag,
                    sizeof( package_actual_tag ) ) ) {
                printf( "Create latest tag: %s "
                        "~instead of latest (?newer)\t\t[All good]\n", package_actual_tag );
            } else {
                pr_error( stdout, "Failed to get latest tag for %s/%s,"
                        "Falling back to main branch\t\t[Fail]",
                        repo->user, repo->repo );
                __debug_function(); /* call debugger function */
                use_fallback_branch = 1;
            }
        } else {
            strncpy( package_actual_tag, repo->tag, sizeof( package_actual_tag ) - 1 );
        }

        /* Fallback to main/master branch if latest tag resolution failed */
        if ( use_fallback_branch ) {
            for ( int j = 0; j < 3 && !ret; j++ ) {
                snprintf( put_url, put_size,
                        "https://github.com/"
                        "%s/%s/archive/refs/heads/%s.zip",
                        repo->user, repo->repo, package_repo_branch[j] );

                if ( package_url_checking( put_url, wgconfig.wg_toml_github_tokens ) ) {
                    ret = 1;
                    if ( j == 1 ) {
                            printf( "Create master branch "
                                "(main branch not found)"
                                "\t\t[All good]\n" );
                    }
                }
            }
            return ret;
        }

        /* Process tagged releases */
        if ( package_actual_tag[0] ) {
            char *package_assets[10] = { 0 };
            int asset_counts = 0;
            asset_counts = package_gh_release_assets( repo->user,
                        repo->repo, package_actual_tag,
                        package_assets, 10 );

            /* If release has downloadable assets */
            if ( asset_counts > 0 ) {
                char *package_best_asset = NULL;
                
                /* Determine target OS based on platform defines */
                const char *target_os = NULL;
                #ifdef WG_WINDOWS
                target_os = "windows";
                #else
                target_os = "linux";
                #endif
                
                package_best_asset = package_fetching_assets( package_assets, asset_counts, target_os );

                if ( package_best_asset ) {
                    strncpy( put_url, package_best_asset, put_size - 1 );
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
                for ( int j = 0; j < asset_counts; j++ )
                    wg_free( package_assets[j] );
            }

            /* If no assets found, try standard archive formats */
            if ( !ret ) {
                const char *package_arch_format[] = {
                    "https://github.com/"
                    "%s/%s/archive/refs/tags/%s.tar.gz",
                    "https://github.com/"
                    "%s/%s/archive/refs/tags/%s.zip"
                };

                for ( int j = 0; j < 2 && !ret; j++ ) {
                    snprintf( put_url, put_size, package_arch_format[j],
                                    repo->user,
                                    repo->repo,
                                    package_actual_tag );

                    if ( package_url_checking( put_url, wgconfig.wg_toml_github_tokens ) )
                        ret = 1;
                }
            }
        } else {
            /* No tag specified - try main/master branches */
            for ( int j = 0; j < 2 && !ret; j++ ) {
                snprintf( put_url, put_size,
                        "%s%s/%s/archive/refs/heads/%s.zip",
                        "https://github.com/",
                        repo->user,
                        repo->repo,
                        package_repo_branch[j] );

                if ( package_url_checking( put_url, wgconfig.wg_toml_github_tokens ) ) {
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
int package_set_hash( const char *raw_file_path, const char *raw_json_path ) {

        char res_convert_f_path[WG_PATH_MAX],
             res_convert_json_path[WG_PATH_MAX];

        /* Convert paths to consistent format */
        strncpy( res_convert_f_path, raw_file_path, sizeof( res_convert_f_path ) );
        res_convert_f_path[sizeof( res_convert_f_path ) - 1] = '\0';
        package_sym_convert( res_convert_f_path );
        
        strncpy( res_convert_json_path, raw_json_path, sizeof( res_convert_json_path ) );
        res_convert_json_path[sizeof( res_convert_json_path ) - 1] = '\0';
        package_sym_convert( res_convert_json_path );

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
 * Updates plugins line with new dependency
 * Replaces struct dencyconfig with direct parameters
 */
void package_implementation_samp_conf( const char* config_file, const char* fw_line, const char* plugin_name ) {

        srand ( ( unsigned int )time(NULL) ^ rand());
        int rand7 = rand () % 10000000;

        char temp_path[ WG_PATH_MAX ];
        snprintf( temp_path, sizeof( temp_path ),
                ".watchdogs/000%07d_temp", rand7 );

        FILE* temp_file = NULL;
        temp_file = fopen( temp_path, "w" );
        
        if (wg_server_env() != 1)
                return;

        if (dir_exists( ".watchdogs" ) == 0)
                MKDIR( ".watchdogs" );

        pr_color( stdout, FCOLOUR_GREEN,
                "Create Dependencies '%s' into '%s'\t\t[All good]\n",
                plugin_name, config_file );

        FILE* ctx_file = fopen( config_file, "r" );
        if (ctx_file) {
                char ctx_line[WG_PATH_MAX];
                int t_exist = 0, tr_exist = 0, tr_ln_has_tx = 0;

                while ( fgets ( ctx_line, sizeof (ctx_line ), ctx_file ) ) {
                        ctx_line[ strcspn ( ctx_line, "\n" ) ] = 0;
                        if (strstr( ctx_line, plugin_name ) != NULL) {
                                t_exist = 1;
                        }
                        if (strstr( ctx_line, fw_line ) != NULL) {
                                tr_exist = 1;
                                if (strstr( ctx_line, plugin_name ) != NULL) {
                                        tr_ln_has_tx = 1;
                                }
                        }
                }
                fclose( ctx_file );

                if ( t_exist ) {
                        return;
                }

                if (tr_exist && !tr_ln_has_tx) {
                        ctx_file = fopen( config_file, "r" );

                        while ( fgets ( ctx_line, sizeof( ctx_line ), ctx_file) ) {
                                char clean_line[ WG_PATH_MAX ];
                                strcpy( clean_line, ctx_line );
                                clean_line[ strcspn ( clean_line, "\n" ) ] = 0;

                                if (strstr( clean_line, fw_line ) != NULL &&
                                    strstr( clean_line, plugin_name ) == NULL)
                                {
                                        fprintf( temp_file,
                                                "%s %s\n", clean_line, plugin_name );
                                } else {
                                        fputs( ctx_line, temp_file );
                                }
                        }

                        fclose( ctx_file );
                        fclose( temp_file );

                        remove( config_file );
                        rename( ".watchdogs/depends_tmp", config_file );
                } else if (!tr_exist) {
                        ctx_file = fopen( config_file, "a" );
                        fprintf( ctx_file, "%s %s\n",
                                        fw_line, plugin_name );
                        fclose( ctx_file );
                }
        } else {
                ctx_file = fopen( config_file, "w" );
                fprintf( ctx_file, "%s %s\n",
                                fw_line, plugin_name );
                fclose( ctx_file );
        }

        return;
}

/* Updated macro for adding SA-MP plugins */
#define S_ADD_PLUGIN(config_file, fw_line, plugin_name) \
        package_implementation_samp_conf(config_file, fw_line, plugin_name)

/* 
 * Add plugin dependency to open.mp JSON configuration
 * Updates legacy_plugins array in server configuration
 */
void package_implementation_omp_conf( const char* config_name, const char* package_name ) {

        if ( wg_server_env() != 2 )
                return;

        pr_color( stdout, FCOLOUR_GREEN,
                "Create Dependencies '%s' into '%s'\t\t[All good]\n",
                package_name, config_name );

        FILE* ctx_file = fopen( config_name, "r" );
        cJSON* s_root = NULL;

        if ( !ctx_file ) {
                s_root = cJSON_CreateObject();
        } else {
                fseek( ctx_file, 0, SEEK_END );
                long fle_size = ftell( ctx_file );
                fseek( ctx_file, 0, SEEK_SET );

                char* buffer = ( char* )wg_malloc( fle_size + 1 );
                if ( !buffer ) {
                        pr_error( stdout, "Memory allocation failed!" );
                        __debug_function(); /* call debugger function */
                        fclose( ctx_file );
                        return;
                }

                size_t file_read = fread( buffer, 1, fle_size, ctx_file );
                if ( file_read != fle_size ) {
                        pr_error( stdout, "Failed to read the entire file!" );
                        __debug_function(); /* call debugger function */
                        wg_free( buffer );
                        fclose( ctx_file );
                        return;
                }

                buffer[fle_size] = '\0';
                fclose( ctx_file );

                s_root = cJSON_Parse( buffer );
                wg_free( buffer );

                if ( !s_root ) {
                        s_root = cJSON_CreateObject();
                }
        }

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
                                "legacy_plugins", legacy_plugins );
        }

        if ( !cJSON_IsArray( legacy_plugins ) ) {
                cJSON_DeleteItemFromObject( pawn, "legacy_plugins" );
                legacy_plugins = cJSON_CreateArray();
                cJSON_AddItemToObject( pawn,
                                "legacy_plugins", legacy_plugins );
        }

        cJSON* dir_item;
        int p_exist = 0;

        cJSON_ArrayForEach( dir_item, legacy_plugins ) {
                if ( cJSON_IsString( dir_item ) &&
                        !strcmp( dir_item->valuestring, package_name ) ) {
                        p_exist = 1;
                        break;
                }
        }

        if ( !p_exist ) {
                cJSON *size_package_name = cJSON_CreateString( package_name );
                cJSON_AddItemToArray( legacy_plugins, size_package_name );
        }

        char* __printed = cJSON_Print( s_root );
        ctx_file = fopen( config_name, "w" );
        if ( ctx_file ) {
                fputs( __printed, ctx_file );
                fclose( ctx_file );
        }

        cJSON_Delete( s_root );
        wg_free( __printed );

        return;
}
/* Macro for adding open.mp plugins */
#define M_ADD_PLUGIN( x, y ) package_implementation_omp_conf( x, y )

/* 
 * Add include directive to gamemode file
 * Inserts #include statement in appropriate location
 */
void package_add_include( const char *modes, char *package_name, char *package_following ) {

        if ( path_exists( modes ) == 0 ) return;
        
        FILE *m_file = fopen( modes, "rb" );
        if ( !m_file ) return;
        
        fseek( m_file, 0, SEEK_END );
        long fle_size = ftell( m_file );
        fseek( m_file, 0, SEEK_SET );
        
        char *ct_modes = wg_malloc( fle_size + 2 );
        if ( !ct_modes ) { fclose(m_file); return; }
        
        size_t bytes_read;
        bytes_read = fread( ct_modes, 1, fle_size, m_file );
        if ( bytes_read != fle_size ) {
                pr_error( stdout, "Failed to read the entire file!" );
                __debug_function(); /* call debugger function */
                wg_free( ct_modes );
                fclose( m_file );
                return;
        }

        ct_modes[fle_size] = '\0';
        fclose( m_file );
        
        char search_name[WG_PATH_MAX];
        strncpy( search_name, package_name, sizeof(search_name)-1 );
        
        char *open = strchr( search_name, '<' );
        char *close = strchr( search_name, '>' );
        if ( open && close ) {
            memmove( search_name, open+1, close-open-1 );
            search_name[close-open-1] = '\0';
        }
        
        char *pos = ct_modes;
        while ( (pos = strstr(pos, "#include")) ) {
            char *line_end = strchr(pos, '\n');
            if (!line_end) line_end = ct_modes + fle_size;
            
            char line[256];
            int len = line_end - pos;
            if (len >= 256) len = 255;
            strncpy(line, pos, len);
            line[len] = '\0';
            
            if (strstr(line, search_name)) {
                wg_free(ct_modes);
                return;
            }
            pos = line_end + 1;
        }
        
        char *insert_at = NULL;
        pos = ct_modes;
        
        while ( (pos = strstr(pos, package_following)) ) {
            char *line_start = pos;
            while (line_start > ct_modes && *(line_start-1) != '\n') line_start--;
            
            char *line_end = strchr(pos, '\n');
            if (!line_end) line_end = ct_modes + fle_size;
            
            char line[256];
            int len = line_end - line_start;
            if (len >= 256) len = 255;
            strncpy(line, line_start, len);
            line[len] = '\0';
            
            if (strstr(line, package_following)) {
                insert_at = line_end;
                break;
            }
            pos = line_end + 1;
        }
        
        FILE *n_file = fopen( modes, "w" );
        if (!n_file) { wg_free(ct_modes); return; }
        
        if (insert_at) {
            fwrite(ct_modes, 1, insert_at - ct_modes, n_file);
            
            if (insert_at > ct_modes && *(insert_at-1) != '\n') {
                fprintf(n_file, "\n");
            }
            
            fprintf(n_file, "%s\n", package_name);
            
            if (*insert_at) {
                char *end = ct_modes + fle_size;
                while (end > insert_at && 
                    (*(end-1) == '\n' || *(end-1) == '\r' || 
                        *(end-1) == ' ' || *(end-1) == '\t')) {
                    end--;
                }
                if (end > insert_at) {
                    fwrite(insert_at, 1, end - insert_at, n_file);
                }
                fprintf(n_file, "\n");
            }
        } else {
            char *last_inc = NULL;
            char *last_inc_end = NULL;
            pos = ct_modes;
            
            while ( (pos = strstr(pos, "#include")) ) {
                last_inc = pos;
                last_inc_end = strchr(pos, '\n');
                if (!last_inc_end) last_inc_end = ct_modes + fle_size;
                pos = last_inc_end + 1;
            }
            
            if (last_inc_end) {
                fwrite(ct_modes, 1, last_inc_end - ct_modes, n_file);
                fprintf(n_file, "\n%s\n", package_name);
                
                char *end = ct_modes + fle_size;
                while (end > last_inc_end && 
                    (*(end-1) == '\n' || *(end-1) == '\r')) {
                    end--;
                }
                if (end > last_inc_end) {
                    fwrite(last_inc_end, 1, end - last_inc_end, n_file);
                }
            } else {
                fprintf(n_file, "%s\n", package_name);
                fwrite(ct_modes, 1, fle_size, n_file);
            }
        }
        
        fclose(n_file);
        wg_free(ct_modes);
}

/* Macro for adding include directives */
#define DENCY_ADD_INCLUDES( x, y, z ) package_add_include( x, y, z )

/* 
 * Process and add include directive based on dependency
 * Reads TOML configuration to determine gamemode file
 */
static void package_include_prints( const char *package_include ) {

        char wg_buf_err[WG_PATH_MAX], dependencies[WG_PATH_MAX],
             _directive[WG_MAX_PATH];
        toml_table_t *wg_toml_config;

        /* Extract filename from path */
        const char *package_n = package_get_filename( package_include );
        snprintf( dependencies, sizeof( dependencies ), "%s", package_n );

        const char *direct_bnames = package_get_basename( dependencies );

        /* Parse TOML configuration */
        FILE *this_proc_file = fopen( "watchdogs.toml", "r" );
        wg_toml_config = toml_parse_file( this_proc_file, wg_buf_err, sizeof( wg_buf_err ) );
        if ( this_proc_file ) fclose( this_proc_file );

        if ( !wg_toml_config ) {
                pr_error( stdout, "failed to parse the watchdogs.toml....: %s", wg_buf_err );
                __debug_function(); /* call debugger function */
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
        snprintf( _directive, sizeof( _directive ),
                "#include <%s>",
                direct_bnames );

        /* Add include based on server environment */
        if ( wg_server_env() == 1 ) {
                /* SA-MP: include after a_samp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                _directive,
                                "#include <a_samp>" );
        } else if ( wg_server_env() == 2 ) {
                /* open.mp: include after open.mp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                _directive,
                                "#include <open.mp>" );
        } else {
                /* Default: include after a_samp */
                DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
                                _directive,
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
        ++fdir_counts;
#if defined(_DBG_PRINT)
        println(stdout, "found (%d): %d", fdir_counts, found);
#endif
        if ( found ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        package_names = package_get_filename( wgconfig.wg_sef_found_list[i] );
                        basename = package_get_basename( wgconfig.wg_sef_found_list[i] );
                        
                        /* Convert to lowercase for case-insensitive comparison */
                        basename_lower = strdup( basename );
                        for ( int j = 0; basename_lower[j]; j++ )
                                {
                                        basename_lower[j] = tolower( basename_lower[j] );
                                }
                                
                        int rate_has_prefix = 0;

                        /* toml parsing a value */
                        pr_info(stdout, "Loaded root patterns: %s", wgconfig.wg_toml_root_patterns);

                        const char* matching_root_keywords = wgconfig.wg_toml_root_patterns;
                        while ( *matching_root_keywords ) {
                                while ( *matching_root_keywords == ' ' /* free */ )
                                        matching_root_keywords++;
                                
                                const char* end = matching_root_keywords;
                                while ( *end && *end != ' ' )
                                        end++;
                                
                                if ( end > matching_root_keywords ) {
                                   size_t keyword_len = end - matching_root_keywords;
                                   if ( strncmp( basename_lower,
                                        matching_root_keywords,
                                        keyword_len
                                        ) == 0)
                                   {
                                      ++rate_has_prefix;
                                      break;
                                   }
                                }
                                
                                matching_root_keywords = ( *end ) ? end + 1 : end;
                        }

                        /* Move to target directory if specified */
                        if ( dump_place[0] != '\0' ) {
                        #ifdef WG_WINDOWS
                                snprintf( command, sizeof( command ),
                                        "move "
                                        "/Y \"%s\" \"%s\\%s\\\"",
                                        wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );

                                wg_run_command( command );
                        #else
                                snprintf( command, sizeof( command ),
                                        "mv "
                                        "-f \"%s\" \"%s/%s/\"",
                                        wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );

                                wg_run_command( command );
                        #endif
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s - %s\n",
                                                 wgconfig.wg_sef_found_list[i], dump_pwd, dump_place );
                        } else {/* Move to current directory */
                                if ( rate_has_prefix ) {
                                #ifdef WG_WINDOWS
                                        snprintf( command, sizeof( command ),
                                                "move "
                                                "/Y \"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], dump_pwd );

                                        wg_run_command( command );
                                #else
                                        snprintf( command, sizeof( command ),
                                                "mv "
                                                "-f \"%s\" \"%s\"",
                                                wgconfig.wg_sef_found_list[i], dump_pwd );

                                        wg_run_command( command );
                                #endif
                                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\n",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                } else {
                                        if ( path_exists( "plugins" ) == 1 ) {
                                        #ifdef WG_WINDOWS
                                                snprintf( command, sizeof( command ),
                                                        "move "
                                                        "/Y \"%s\" \"%s\\plugins\"",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );

                                                wg_run_command( command );

                                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\\plugins\n",
                                                                wgconfig.wg_sef_found_list[i], dump_pwd );
                                        #else
                                                snprintf( command, sizeof( command ),
                                                        "mv "
                                                        "-f \"%s\" \"%s/plugins\"",
                                                        wgconfig.wg_sef_found_list[i], dump_pwd );
                                                
                                                wg_run_command( command );

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
                                package_set_hash( json_item, json_item );

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
void package_cjson_additem( cJSON *p1, int p2, cJSON *p3 )
{
        if ( cJSON_IsString( cJSON_GetArrayItem( p1, p2 ) ) )
                cJSON_AddItemToArray( p3,
                        cJSON_CreateString( cJSON_GetArrayItem( p1, p2 )->valuestring ) );
}

/* 
 * Organize dependency files into appropriate directories
 * Moves plugins, includes, and components to their proper locations
 */
void package_move_files( const char *package_dir )
{
        /* Char */
        char root_dir[WG_PATH_MAX], includes[WG_MAX_PATH],
             plugins[WG_PATH_MAX], components[WG_PATH_MAX],
             the_path[WG_PATH_MAX * 2], full_path[WG_PATH_MAX],
             src[WG_PATH_MAX], dest[WG_MAX_PATH * 2];
        char *pos;

        /* Pointer */
        const char
           /* const */ *packages;
        char *include_path = NULL;
        char *src_sep = NULL;

        /* Int */
        int i;
        /* Stack */
        int _sindex = -1;
        int _ssize = WG_MAX_PATH;
        int rate_found_include = 0;
        
        /* Reset */
        fdir_counts = 0;

        /* CWD/PWD */
        char *fetch_pwd = NULL;
        fetch_pwd = wg_procure_pwd();

        /* Stack-based directory traversal for finding nested include files */
        struct stat dir_st;
        struct dirent *dir_item;
        char
                **tmp_stack = wg_malloc( _ssize * sizeof( char* ) );
        if ( tmp_stack == 0 )
                {
                        return;
                }
        char **stack = tmp_stack;

        for ( i = 0; i < _ssize; i++ )
                {
                        stack[i] = wg_malloc( WG_PATH_MAX );
                }
        
        /* Construct platform-specific paths */
        #ifdef WG_WINDOWS
        snprintf( plugins, sizeof( plugins ), "%s\\plugins",
                package_dir );
        snprintf( components, sizeof( components ), "%s\\components",
                package_dir );
        #else
        snprintf( plugins, sizeof( plugins ), "%s/plugins",
                package_dir );
        snprintf( components, sizeof( components ), "%s/components",
                package_dir );
        #endif

#if defined(_DBG_PRINT)
        println(stdout, "plug path: %s", plugins);
#endif

#if defined(_DBG_PRINT)
        println(stdout, "comp path: %s", components);
#endif

        /* Determine include path based on server environment */
        if ( wg_server_env() == 1 ) {
        #ifdef WG_WINDOWS
                include_path = "pawno\\include";
                snprintf( full_path, sizeof( full_path ), "%s\\pawno\\include", package_dir );
        #else
                include_path = "pawno/include";
                snprintf( full_path,  sizeof( full_path ), "%s/pawno/include", package_dir );
        #endif
        } else if ( wg_server_env() == 2 ) {
        #ifdef WG_WINDOWS
                include_path = "qawno\\include";
                snprintf( full_path, sizeof( full_path ), "%s\\qawno\\include", package_dir );
        #else
                include_path = "qawno/include";
                snprintf( full_path, sizeof( full_path ), "%s/qawno/include", package_dir );
        #endif
        }

#if defined(_DBG_PRINT)
        println(stdout, "package include path: %s", include_path);
#endif

#if defined(_DBG_PRINT)
        println(stdout, "full path: %s", full_path);
#endif

        wg_sef_fdir_memset_to_null();

        /* Move include files from standard include directory */
        rate_found_include = wg_sef_fdir( full_path, "*.inc", NULL );
#if defined(_DBG_PRINT)
        println(stdout, "package inc: %d", rate_found_include);
#endif
        if ( rate_found_include ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        packages = package_get_filename( wgconfig.wg_sef_found_list[i] );
                #ifdef WG_WINDOWS
                        snprintf( command, sizeof( command ),
                                "move "
                                "/Y \"%s\" \"%s\\%s\\\"",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );

                        wg_run_command( command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );
                #else
                        snprintf( command, sizeof( command ),
                                "mv "
                                "-f \"%s\" \"%s/%s/\"",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );

                        wg_run_command( command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );
                #endif

                        package_set_hash( packages,
                                packages );
                        package_include_prints( packages );
                }
        }

        wg_sef_fdir_memset_to_null();

        /* Move include files from root of package_dir */
        rate_found_include = wg_sef_fdir( package_dir, "*.inc", NULL );
#if defined(_DBG_PRINT)
        println(stdout, "package inc2: %d", rate_found_include);
#endif
        if ( rate_found_include ) {
                for ( i = 0; i < wgconfig.wg_sef_count; ++i ) {
                        const char *packages;
                        packages = package_get_filename( wgconfig.wg_sef_found_list[i] );

                #ifdef WG_WINDOWS
                        snprintf( command, sizeof( command ),
                                "move "
                                "/Y \"%s\" \"%s\\%s\\\"",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );

                        wg_run_command( command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );
                #else
                        snprintf( command, sizeof( command ),
                                "mv "
                                "-f \"%s\" \"%s/%s/\"",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );

                        wg_run_command( command );

                        pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
                                wgconfig.wg_sef_found_list[i], fetch_pwd, include_path );
                #endif

                        package_set_hash( packages,
                                packages );
                        package_include_prints( packages );
                }
        }

        /* Normalize path separators */
        while ( ( pos = strstr( include_path, "include\\" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include\\" ) ) + 1 );
        while ( ( pos = strstr( include_path, "include/" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include/" ) ) + 1 );
        while ( ( pos = strstr( full_path, "include\\" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include\\" ) ) + 1 );
        while ( ( pos = strstr( full_path, "include/" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include/" ) ) + 1 );

        /* Move plugin files */
#ifndef WG_WINDOWS
        dump_file_type( plugins, "*.dll", NULL, fetch_pwd, "/plugins", 0 );
        dump_file_type( plugins, "*.so", NULL, fetch_pwd, "/plugins", 0 );
#else
        dump_file_type( plugins, "*.dll", NULL, fetch_pwd, "\\plugins", 0 );
        dump_file_type( plugins, "*.so", NULL, fetch_pwd, "\\plugins", 0 );
#endif

#if defined(_DBG_PRINT)
        println(stdout, "plug path: %s", plugins);
#endif
        /* Move root-level plugin files */
        dump_file_type( package_dir, "*.dll", "plugins", fetch_pwd, "", 1 );
        dump_file_type( package_dir, "*.so", "plugins", fetch_pwd, "", 1 );

        /* Handle open.mp specific components */
        if ( wg_server_env() == 2 ) {
        #ifdef WG_WINDOWS
                snprintf( includes, sizeof( includes ), "qawno\\include" );
        #else
                snprintf( includes, sizeof( includes ), "qawno/include" );
        #endif
                dump_file_type( components, "*.dll", NULL, fetch_pwd, "components", 0 );
                dump_file_type( components, "*.so", NULL, fetch_pwd, "components", 0 );
        } else {
        #ifdef WG_WINDOWS
                snprintf( includes, sizeof( includes ), "pawno\\include" );
        #else
                snprintf( includes, sizeof( includes ), "pawno/include" );
        #endif
        }

        /* Normalize include path */
        while ( ( pos = strstr( includes, "include\\" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include\\" ) ) + 1 );
        while ( ( pos = strstr( includes, "include/" ) ) != NULL )
                memmove( pos, pos + strlen( "include/" ),
                        strlen( pos + strlen( "include/" ) ) + 1 );

        /* Start with dependency directory */
        ++_sindex;
        snprintf( stack[_sindex], WG_PATH_MAX, "%s", package_dir );

        /* Depth-first search for include files */
        while ( _sindex >= 0 ) {
                strlcpy( root_dir, stack[_sindex], sizeof( root_dir ) );
                --_sindex;

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
                        strlcpy( the_path,
                                root_dir, sizeof( the_path ) );
                        strlcat( the_path,
                                __PATH_STR_SEP_LINUX, sizeof( the_path ) );
                        strlcat( the_path,
                                dir_item->d_name, sizeof( the_path ) );

                        if ( stat( the_path, &dir_st ) != 0 )
                                {
                                        continue;
                                }
                                
                        if ( S_ISDIR( dir_st.st_mode ) ) {
                                /* Skip compiler directories */
                                if ( strcmp( dir_item->d_name, "pawno" ) == 0 ||
                                    strcmp( dir_item->d_name, "qawno" ) == 0 ||
                                    strcmp( dir_item->d_name, "include" ) == 0 ||
                                    strcmp ( dir_item->d_name, "components") == 0 ||
                                    strcmp ( dir_item->d_name, "plugins") == 0)
                                {
                                        continue;
                                }
                                /* Push directory onto stack for later processing */
                                if ( _sindex < _ssize - 1 )
                                        {
                                                ++_sindex;
                                                strlcpy( stack[_sindex],
                                                        the_path, WG_MAX_PATH );
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
                        src_sep = strrchr( src, __PATH_CHR_SEP_WIN32 );
                        #else
                        src_sep = strrchr( src, __PATH_CHR_SEP_LINUX );
                        #endif
                        if ( !src_sep )
                                {
                                        continue;
                                }
                        ++src_sep;

                        /* Construct destination path */
                        strlcpy( dest, includes, sizeof( dest ) );
                        #ifdef WG_WINDOWS
                        strlcat( dest, __PATH_STR_SEP_WIN32, sizeof( dest ) );
                        #else
                        strlcat( dest, __PATH_STR_SEP_LINUX, sizeof( dest ) );
                        #endif
                        strlcat( dest, src_sep, sizeof( dest ) );

                        /* Move or copy directory containing include files */
                        if ( rename( src, dest ) ) {
                        #ifdef WG_WINDOWS
                                /* Windows: copy directory tree and remove original */
                                const char *windows_move_parts[] = {
                                        "xcopy ", src, " ", dest, " /E /I /H /Y " ">nul 2>&1 && " "rmdir /S /Q ", src, " >nul 2>&1"
                                };
                                size_t size_windows_move_parts = sizeof( windows_move_parts ),
                                       size_windows_move_parts_zero = sizeof( windows_move_parts[0] );
                                strlcpy( command, "", sizeof( command ) );
                                int j;
                                for ( j = 0;
                                      j < size_windows_move_parts / size_windows_move_parts_zero;
                                      j++ )
                                {
                                        strlcat( command, windows_move_parts[j], sizeof( command ) );
                                }
                                char *size_src = strdup(src);
                                for ( pos = size_src; *pos; pos++ ) if ( *pos == __PATH_CHR_SEP_LINUX )
                                {
                                        *pos = __PATH_CHR_SEP_WIN32;
                                }
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s\\?\n",
                                                 size_src, dest );
                        #else
                                /* Unix: recursive copy and remove */
                                const char *unix_move_parts[] = {
                                        "cp -r ", src, " ", dest, " && rm -rf ", src
                                };
                                size_t size_unix_move_parts = sizeof( unix_move_parts ),
                                       size_unix_move_parts_zero = sizeof( unix_move_parts[0] );
                                strlcpy( command, "", sizeof( command ) );
                                int j;
                                for ( j = 0;
                                      j < size_unix_move_parts / size_unix_move_parts_zero;
                                      j++ )
                                {
                                        strlcat( command, unix_move_parts[j], sizeof( command ) );
                                }
                                pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s/?\n",
                                                 src, dest );
                        #endif
                        }

                        package_set_hash( dest, dest );
                }

                closedir( open_dir );
        }

        /* Clean up directory stack */
        for ( i = 0;
              i < _ssize;
              i++ ) {
                wg_free( stack [ i ] );
        }
        wg_free( stack );

        printf( "\n" );

        /* Remove extracted dependency directory */
        #ifdef WG_WINDOWS
        snprintf( command, sizeof( command ),
                "rmdir "
                "/s "
                "/q \"%s\"",
                package_dir );
        #else
        snprintf( command, sizeof( command ),
                "rm "
                "-rf %s",
                package_dir );
        #endif
        wg_run_command( command );

        return;
}

/* 
 * Prepare and organize dependencies after download
 * Creates necessary directories and initiates file organization
 */
void wg_apply_depends( const char *depends_name )
{
        char _dependencies[WG_PATH_MAX];
        char package_dir[WG_PATH_MAX];
        
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

        snprintf( package_dir, sizeof( package_dir ), "%s", _dependencies );

#if defined(_DBG_PRINT)
        println(stdout, "dency dir: %s", package_dir);
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
        package_move_files( package_dir );
}

/* 
 * Main dependency installation function
 * Parses dependency strings, downloads archives, and applies dependencies
 */
void wg_install_depends( const char *dependencies_str, const char *branch )
{
        char buffer[1024];
        char package_url[1024];
        char *procure_buffer;
        struct repositories repo;
        char package_name[WG_PATH_MAX];
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
                if ( !package_parse_repo( dependencies[i], &repo ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "invalid repo format: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                
                /* Handle GitHub repositories */
                if ( !strcmp( repo.host, "github" ) ) {
                        if ( !package_handle_repo( &repo, package_url,
                            sizeof( package_url ),
                            branch ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "repo not found: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                } else {
                        /* Handle custom repositories */
                        package_build_repo_url( &repo, 0, package_url, sizeof( package_url ) );
                        if ( !package_url_checking( package_url,
                            wgconfig.wg_toml_github_tokens ) )
                        {
                                pr_color( stdout, FCOLOUR_RED, "" );
                                printf( "repo not found: %s"
                                        "\t\t[Fail]\n", dependencies[i] );
                                continue;
                        }
                }

                /* Extract filename from URL */
                if ( strrchr( package_url, __PATH_CHR_SEP_LINUX ) &&
                    *( strrchr( package_url, __PATH_CHR_SEP_LINUX ) + 1 ) )
                {
                        snprintf( package_name, sizeof( package_name ), "%s",
                                strrchr( package_url, __PATH_CHR_SEP_LINUX ) + 1 );

                        /* Ensure archive extension */
                        if ( !strend( package_name, ".tar.gz", true ) &&
                            !strend( package_name, ".tar", true ) &&
                            !strend( package_name, ".zip", true ) )
                        {
                                snprintf( package_name + strlen( package_name ),
                                        sizeof( package_name ) - strlen( package_name ), ".zip" );
                        }
                } else {
                        /* Default naming scheme */
                        snprintf( package_name, sizeof( package_name ), "%s.tar.gz", repo.repo );
                }

                if ( !*package_name ) {
                        pr_color( stdout, FCOLOUR_RED, "" );
                        printf( "invalid repo name: %s\t\t[Fail]\n", package_url );
                        continue;
                }

                wgconfig.wg_idepends = 1;

                /* Time the dependency installation process */
                wg_download_file( package_url, package_name );
                wg_apply_depends( package_name );
        }

done:
        return;
}
