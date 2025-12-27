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
#include "replicate.h"

static char command[ WG_MAX_PATH * 4 ];
static char *tag;
static char *path;
static char *path_slash;
static char *user;
static char *repo;
static char *repo_slash;
static char *git_dir;
static char *filename;
static char *extension;
static char json_item[WG_PATH_MAX];
static int fdir_counts = REPLICATE_RATE_ZERO;

const char *match_windows_lookup_pattern[] = { "windows", "win", "win32", "win32", "msvc", "mingw", ".dll", NULL };
const char *match_linux_lookup_pattern[] = { "linux", "ubuntu", "debian", "cent", "centos", "cent_os", "fedora", "arch", "archlinux", "alphine", "rhel", "redhat", "linuxmint", "mint", ".so", NULL };
const char *match_any_lookup_pattern[] = {
		"src", "source", "proj", "project", "server", "_server", "gamemode",
		"gamemodes", "bin", "build", "packages", "resources", "modules",
		"plugins", "addons", "extensions", "scripts", "system", "core",
		"runtime", "libs", "include", "deps", "dependencies", ".inc", NULL
};

void package_sym_convert( char *path )
{
		char *pos;
		for ( pos = path; *pos; pos++ )
			{
				if ( *pos == __PATH_CHR_SEP_WIN32 )
  				*pos = __PATH_CHR_SEP_LINUX;
  		}
}

int thrate_os_archive( const char *filename ) {

		int k;

		const char *__HOST_OS = NULL;
		#ifdef WG_WINDOWS
		__HOST_OS = "windows";
		#else
		__HOST_OS = "linux";
		#endif

		if ( !__HOST_OS )
				return 0;

		char size_host_os[ WG_PATH_MAX ];
		const char **lookup_pattern = NULL;
		char filename_lwr[ WG_PATH_MAX ];

		strncpy( size_host_os, __HOST_OS, sizeof ( size_host_os ) - 1 );
		size_host_os[ sizeof( size_host_os ) - 1 ] = '\0';

		for (int i = REPLICATE_RATE_ZERO; size_host_os[ i ]; i++) {
		    size_host_os[ i ] = tolower(size_host_os[ i ]);
		}

		if (strfind( size_host_os, "win", true )) lookup_pattern = match_windows_lookup_pattern;
		else if (strfind(size_host_os, "linux", true )) lookup_pattern = match_linux_lookup_pattern;

		if ( !lookup_pattern )
				return 0;

		strncpy( filename_lwr, filename, sizeof ( filename_lwr ) - 1 );
		filename_lwr [sizeof ( filename_lwr ) - 1 ] = '\0';

		for (int i = REPLICATE_RATE_ZERO; filename_lwr[ i ]; i++) {
		    filename_lwr[ i ] = tolower(filename_lwr[ i ]);
		}

		for (k = REPLICATE_RATE_ZERO; lookup_pattern[ k ] != NULL; ++k) {
		    if ( strfind(
				    filename_lwr,
				    lookup_pattern[ k ],
						true)
				) {
				return 1;
		    }
		}

		return 0;
}

int thrate_more_archive(const char *filename)
{
		int k;				     /* loop counter for array _sindex */
		int ret = REPLICATE_RATE_ZERO;		       /* return value, default 0 (not found) */

		/* Iterate through all server lookup_pattern until NULL terminator */
		for (k = REPLICATE_RATE_ZERO;
		     match_any_lookup_pattern[k] != NULL;  /* check array end */
		     ++k) {
		       /* Search for current server pattern in filename */
		       if (strfind(
						filename,		       /* string to search in */
						match_any_lookup_pattern[k],  /* pattern to find */
						true)				   /* case-sensitive flag */
		       ) {
						ret = 1;    /* pattern found, set return to 1 */
						break;      /* exit loop early */
				}
		}

		return ret;  /* return result (0=not found, 1=found) */
}

static char *try_build_os_asseets( char **assets, int count, const char *os_pattern )
{
		int i;

		const char *const *lookup_pattern = NULL;

		if (strfind(os_pattern, "win", true))
			lookup_pattern = match_windows_lookup_pattern;
		else if (strfind(os_pattern, "linux", true))
			lookup_pattern = match_linux_lookup_pattern;
		else
			return NULL;

		for (i = REPLICATE_RATE_ZERO; i < count; i++) {
			const char *asset = assets[i];
			int p;

			for (p = REPLICATE_RATE_ZERO; lookup_pattern[p]; p++) {
				if (!strfind(asset, lookup_pattern[p], true))
					continue;

				return strdup(asset);
			}
		}

		return NULL;
 }

 static char *try_server_assets( char **assets, int count, const char *os_pattern )
 {
		const char *const *os_patterns = NULL;
		int i;

		if (strfind(os_pattern, "win", true))
			os_patterns = match_windows_lookup_pattern;
		else if (strfind(os_pattern, "linux", true))
			os_patterns = match_linux_lookup_pattern;
		else
			return NULL;

		for (i = REPLICATE_RATE_ZERO; i < count; i++) {
		 	const char *asset = assets[i];
		 	int p;

		 	for (p = REPLICATE_RATE_ZERO; match_any_lookup_pattern[p]; p++) {
		 		if (strfind(asset, match_any_lookup_pattern[p], true))
		 			break;
		 	}

		 	if (!match_any_lookup_pattern[p])
		 		continue;

		 	for (p = REPLICATE_RATE_ZERO; os_patterns[p]; p++) {
		 		if (strfind(asset, os_patterns[p], true))
		 			return strdup(asset);
		 	}
		}

		return NULL;
 }

 static char *try_generic_assets( char **assets, int count )
 {
	 	int i;

		for (i = REPLICATE_RATE_ZERO; i < count; i++) {
			const char *asset = assets[i];
			int p;

			for (p = REPLICATE_RATE_ZERO; match_any_lookup_pattern[p]; p++) {
				if (strfind(asset, match_any_lookup_pattern[p], true))
					return strdup(asset);
			}
		}

		for (i = REPLICATE_RATE_ZERO; i < count; i++) {
			const char *asset = assets[i];
			int p;

			for (p = REPLICATE_RATE_ZERO; match_windows_lookup_pattern[p]; p++) {
				if (strfind(asset, match_windows_lookup_pattern[p], true))
					break;
			}

			if (match_windows_lookup_pattern[p])
				continue;

			for (p = REPLICATE_RATE_ZERO; match_linux_lookup_pattern[p]; p++) {
				if (strfind(asset, match_linux_lookup_pattern[p], true))
					break;
			}

			if (match_linux_lookup_pattern[p])
				continue;

			return strdup(asset);
		}

		return strdup(assets[0]);
 }

 char *package_fetching_assets( char **package_assets,
				      int counts, const char *pf_os )
 {
		char *result = NULL;
		char size_host_os[32] = {0};
		const char *__HOST_OS = NULL;

		if (counts == REPLICATE_RATE_ZERO)
			return NULL;
		if (counts == 1)
			return strdup(package_assets[0]);

		if (pf_os && pf_os[0]) {
			__HOST_OS = pf_os;
		} else {
 #ifdef WG_WINDOWS
			__HOST_OS = "windows";
 #else
			__HOST_OS = "linux";
 #endif
		}

		if (__HOST_OS) {
			strncpy(size_host_os, __HOST_OS, sizeof(size_host_os) - 1);
			for (int j = REPLICATE_RATE_ZERO; size_host_os[j]; j++) {
				size_host_os[j] = tolower(size_host_os[j]);
			}
		}

		if (size_host_os[0]) {
			result = try_server_assets(package_assets, counts, size_host_os);
			if (result)
				return result;

			result = try_build_os_asseets(package_assets, counts, size_host_os);
			if (result)
				return result;
		}

		return try_generic_assets(package_assets, counts);
}

static int
package_parse_repo( const char *input, struct _repositories *__package_data ) {

		memset( __package_data, REPLICATE_RATE_ZERO, sizeof( *__package_data ) );

		strcpy( __package_data->host,   "github" );
		strcpy( __package_data->domain, "github.com" );

		char parse_input[WG_PATH_MAX * 2];
		strncpy( parse_input, input, sizeof( parse_input ) - 1 );
		parse_input[sizeof( parse_input ) - 1] = '\0';

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

		if ( !strncmp( path, "https://", 8 ) )
				path += 8;
		else if ( !strncmp( path, "http://", 7 ) )
				path += 7;

		if ( !strncmp( path, "github/", 7 ) ) {
				strcpy( __package_data->host, "github" );
				strcpy( __package_data->domain, "github.com" );
				path += 7;
		} else {
				path_slash = strchr( path, __PATH_CHR_SEP_LINUX );

				if ( path_slash && strchr( path, '.' ) && strchr( path, '.' ) < path_slash ) {
					char domain[WG_PATH_MAX];

					strncpy( domain, path, path_slash - path );
					domain[path_slash - path] = '\0';

					if ( strfind ( domain, "github", true ) ) {
							strcpy( __package_data->host, "github" );
							strcpy( __package_data->domain, "github.com" );
					} else {
							pr_error(stdout, "Currently Watchdogs only supported for GitHub..");
							return 1;
					}

					path = path_slash + 1;
				}
		}

		user = path;
		repo_slash = strchr( path, __PATH_CHR_SEP_LINUX );
		if ( !repo_slash )
				return 0;

		*repo_slash = '\0';
		repo = repo_slash + 1;

		strncpy( __package_data->user,
				user,
				sizeof( __package_data->user ) - 1 );

		char *__GIT = ".git";
		git_dir = strstr( repo, __GIT );

		if ( git_dir ) { *git_dir = '\0'; }
		strncpy( __package_data->repo,
				repo, sizeof( __package_data->repo ) - 1 );

		if ( strlen( __package_data->user ) == REPLICATE_RATE_ZERO ||
		     strlen( __package_data->repo ) == REPLICATE_RATE_ZERO )
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
		int url_count = REPLICATE_RATE_ZERO;

		/* Build GitHub API URL for specific release tag */
		snprintf( api_url, sizeof( api_url ),
						 "%sapi.github.com/repos/%s/%s/releases/tags/%s",
						 "https://", user, repo, tag );

		/* Fetch release information JSON */
		if ( !package_http_get_content( api_url, wgconfig.wg_toml_github_tokens, &json_data ) ) {
				wg_free( json_data );
				return 0;
		}

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
package_build_repo_url( const struct _repositories *__package_data, int rate_tag_page,
				   char *put_url, size_t put_size )
{
		char package_actual_tag[128] = { 0 };

		if ( __package_data->tag[0] ) {
			strncpy( package_actual_tag, __package_data->tag,
					sizeof( package_actual_tag ) - 1 );

			/* Handle "newer" tag special case */
			if ( strcmp( package_actual_tag, "newer" ) == REPLICATE_RATE_ZERO ) {
				if ( strcmp( __package_data->host, "github" ) == REPLICATE_RATE_ZERO && !rate_tag_page ) {
						strcpy( package_actual_tag, "latest" );
				}
			}
		}

		if ( strcmp( __package_data->host, "github" ) == REPLICATE_RATE_ZERO ) {
			if ( rate_tag_page && package_actual_tag[0] ) {
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
					snprintf( put_url, put_size,
							"https://%s/%s/%s/archive/refs/heads/main.zip",
							__package_data->domain, __package_data->user,
							__package_data->repo );
			}
		}
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
		int ret = REPLICATE_RATE_ZERO;

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
package_handle_repo( const struct _repositories *revolver_repos, char *put_url, size_t put_size, const char *branch ) {

		int ret = REPLICATE_RATE_ZERO;

		 /* Common branch name */
		const char *package_repo_branch[] = { branch, "main", "master" };
		char package_actual_tag[128] = { 0 };
		int use_fallback_branch = REPLICATE_RATE_ZERO;

		if ( revolver_repos->tag[0] && strcmp( revolver_repos->tag, "newer" ) == REPLICATE_RATE_ZERO ) {
		    if ( package_gh_latest_tag( revolver_repos->user,
				    revolver_repos->repo,
				    package_actual_tag,
				    sizeof( package_actual_tag ) ) ) {
				printf( "Create latest tag: %s "
						"~instead of latest (?newer)\t\t[All good]\n", package_actual_tag );
		    } else {
				pr_error( stdout, "Failed to get latest tag for %s/%s,"
						"Falling back to main branch\t\t[Fail]",
						revolver_repos->user, revolver_repos->repo );
				__debug_function(); /* call debugger function */
				use_fallback_branch = 1;
		    }
		} else {
		    strncpy( package_actual_tag, revolver_repos->tag, sizeof( package_actual_tag ) - 1 );
		}

		if ( use_fallback_branch ) {
		    for ( int j = REPLICATE_RATE_ZERO; j < 3 && !ret; j++ ) {
				snprintf( put_url, put_size,
						"https://github.com/"
						"%s/%s/archive/refs/heads/%s.zip",
						revolver_repos->user, revolver_repos->repo, package_repo_branch[j] );

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

		pr_info(stdout, "Fetching any archive...");

		/* Process tagged releases */
		if ( package_actual_tag[0] ) {
		    char *package_assets[10] = { 0 };
		    int asset_counts = REPLICATE_RATE_ZERO;
		    asset_counts = package_gh_release_assets( revolver_repos->user,
						revolver_repos->repo, package_actual_tag,
						package_assets, 10 );

		    if ( asset_counts > 0 ) {
				char *package_best_asset = NULL;

				const char *__HOST_OS = NULL;
				#ifdef WG_WINDOWS
				__HOST_OS = "windows";
				#else
				__HOST_OS = "linux";
				#endif

				package_best_asset = package_fetching_assets( package_assets, asset_counts, __HOST_OS );

				if ( package_best_asset ) {
				    strncpy( put_url, package_best_asset, put_size - 1 );
				    ret = 1;

				    /* asset selection */
				    if ( thrate_more_archive( package_best_asset ) ) {
    						; /* empty statement */
				    } else if ( thrate_os_archive( package_best_asset ) ) {
    						; /* empty statement */
				    } else {
    						; /* empty statement */
				    }

				    pr_info(stdout, "Try Archive: %s\t\t[All good]\n",
								package_best_asset );

				    wg_free( package_best_asset );
				}

				/* Clean up asset array */
				for ( int j = REPLICATE_RATE_ZERO; j < asset_counts; j++ )
				    wg_free( package_assets[j] );
		    }

		    if ( !ret ) {
				const char *package_arch_format[] = {
				    "https://github.com/"
				    "%s/%s/archive/refs/tags/%s.tar.gz",
				    "https://github.com/"
				    "%s/%s/archive/refs/tags/%s.zip"
				};

				for ( int j = REPLICATE_RATE_ZERO; j < 2 && !ret; j++ ) {
				    snprintf( put_url, put_size, package_arch_format[j],
								    revolver_repos->user,
								    revolver_repos->repo,
								    package_actual_tag );

				    if ( package_url_checking( put_url, wgconfig.wg_toml_github_tokens ) )
						ret = 1;
				}
		    }
		} else {
		    /* No tag specified - try main/master branches */
		    for ( int j = REPLICATE_RATE_ZERO; j < 2 && !ret; j++ ) {
				snprintf( put_url, put_size,
						"%s%s/%s/archive/refs/heads/%s.zip",
						"https://github.com/",
						revolver_repos->user,
						revolver_repos->repo,
						package_repo_branch[j] );

				if ( package_url_checking( put_url, wgconfig.wg_toml_github_tokens ) ) {
				    ret = 1;
				    if ( j == 1 )
						printf( "Create master branch "
                        "(main branch not found)\t\t"
                        "[All good]\n" );
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
		if ( crypto_generate_sha1_hash( res_convert_json_path, sha1_hash ) == REPLICATE_RATE_ZERO ) {
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
		int int_random_size = rand () % 10000000;

		char temp_path[ WG_PATH_MAX ];
		snprintf( temp_path, sizeof( temp_path ),
				".watchdogs/000%07d_temp", int_random_size );

		FILE* temp_file = NULL;
		temp_file = fopen( temp_path, "w" );

		if (wg_server_env() != 1)
				return;

		if (dir_exists( ".watchdogs" ) == REPLICATE_RATE_ZERO)
				MKDIR( ".watchdogs" );

		pr_color( stdout, FCOLOUR_GREEN, "Create Dependencies '%s' into '%s'\t\t"
                "[All good]\n",
    				plugin_name, config_file );

		FILE* ctx_file = fopen( config_file, "r" );
		if (ctx_file) {
				char ctx_line[WG_PATH_MAX];
				int t_exist = REPLICATE_RATE_ZERO, tr_exist = REPLICATE_RATE_ZERO, tr_ln_has_tx = REPLICATE_RATE_ZERO;

				while ( fgets ( ctx_line, sizeof (ctx_line ), ctx_file ) ) {
						ctx_line[ strcspn ( ctx_line, "\n" ) ] = REPLICATE_RATE_ZERO;
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
								clean_line[ strcspn ( clean_line, "\n" ) ] = REPLICATE_RATE_ZERO;

								if (strstr( clean_line, fw_line ) != NULL &&
								    strstr( clean_line, plugin_name ) == NULL)
								{
										fprintf( temp_file,
												"%s" " %s\n", clean_line, plugin_name );
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

		pr_color( stdout, FCOLOUR_GREEN, "Create Dependencies '%s' into '%s'\t\t"
              "[All good]\n",
  				package_name, config_name );

		FILE* ctx_file = fopen( config_name, "r" );
		cJSON* s_root = NULL;

		if ( !ctx_file ) {
				s_root = cJSON_CreateObject();
		} else {
				fseek( ctx_file, REPLICATE_RATE_ZERO, SEEK_END );
				long fle_size = ftell( ctx_file );
				fseek( ctx_file, REPLICATE_RATE_ZERO, SEEK_SET );

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
		int p_exist = REPLICATE_RATE_ZERO;

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

		if ( path_exists( modes ) == REPLICATE_RATE_ZERO ) return;

		FILE *m_file = fopen( modes, "rb" );
		if ( !m_file ) return;

		fseek( m_file, REPLICATE_RATE_ZERO, SEEK_END );
		long fle_size = ftell( m_file );
		fseek( m_file, REPLICATE_RATE_ZERO, SEEK_SET );

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
		const char *package_n = PACKAGE_GET_FILENAME( package_include );
		snprintf( dependencies, sizeof( dependencies ), "%s", package_n );

		const char *direct_bnames = PACKAGE_GET_BASENAME( dependencies );

		/* Parse TOML configuration */
		FILE *thrate_proc_file = fopen( "watchdogs.toml", "r" );
		wg_toml_config = toml_parse_file( thrate_proc_file, wg_buf_err, sizeof( wg_buf_err ) );
		if ( thrate_proc_file ) fclose( thrate_proc_file );

		if ( !wg_toml_config ) {
				pr_error( stdout, "failed to parse the watchdogs.toml...: %s", wg_buf_err );
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
				"#include <%s>", direct_bnames );

		/* Add include based on server environment */
		if ( wg_server_env() == 1 ) {
				/* SA-MP: include after a_samp */
				DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
								_directive, "#include <a_samp>" );
		} else if ( wg_server_env() == 2 ) {
				/* open.mp: include after open.mp */
				DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
								_directive, "#include <open.mp>" );
		} else {
				/* Default: include after a_samp */
				DENCY_ADD_INCLUDES( wgconfig.wg_toml_proj_input,
								_directive, "#include <a_samp>" );
		}
}

/*
 * Search for and move files matching pattern
 * Handles file relocation and configuration updates
 */
void dump_file_type( const char *dump_path, char *dump_pattern,
				    char *dump_exclude, char *dump_loc, char *dump_place, int dump_root )
{
		wg_sef_fdir_memset_to_null();

		const char *package_names, *basename;
		char *basename_lwr;

		/* Search for files matching dump_pattern */
		int i;
		int found = -1;
		found = wg_sef_fdir( dump_path, dump_pattern, dump_exclude );
		++fdir_counts;
#if defined(_DBG_PRINT)
		println(stdout, "fdir_counts (%d): %d", fdir_counts, found);
#endif
		if ( found ) {
			for ( i = REPLICATE_RATE_ZERO; i < wgconfig.wg_sef_count; ++i ) {
				package_names = PACKAGE_GET_FILENAME( wgconfig.wg_sef_found_list[i] );
				basename = PACKAGE_GET_BASENAME( wgconfig.wg_sef_found_list[i] );

				/* Convert to lowercase for case-insensitive comparison */
				basename_lwr = strdup( basename );
				for ( int j = REPLICATE_RATE_ZERO; basename_lwr[j]; j++ )
					{
							basename_lwr[j] = tolower( basename_lwr[j] );
					}

				int rate_has_prefix = REPLICATE_RATE_ZERO;

				/* toml parsing a value */
				pr_info(stdout,
					"Loaded root patterns: " FCOLOUR_CYAN "%s", wgconfig.wg_toml_root_patterns);

				const char* match_root_keywords = wgconfig.wg_toml_root_patterns;
				while ( *match_root_keywords ) {
						while ( *match_root_keywords == ' ' /* free */ )
								match_root_keywords++;

						const char* end = match_root_keywords;
						while ( *end && *end != ' ' )
								end++;

						if ( end > match_root_keywords ) {
						   size_t keyword_len = end - match_root_keywords;
						   if ( strncmp( basename_lwr,
								match_root_keywords,
								keyword_len
								) == REPLICATE_RATE_ZERO)
						   {
						      ++rate_has_prefix;
						      break;
						   }
						}

						match_root_keywords = ( *end ) ? end + 1 : end;
				}
				wg_free(basename_lwr);

				/* Move to target directory if specified */
				if ( dump_place[0] != '\0' ) {
				#ifdef WG_WINDOWS
						snprintf( command, sizeof( command ),
								"move "
								"/Y \"%s\" \"%s\\%s\\\"",
								wgconfig.wg_sef_found_list[i], dump_loc, dump_place );

						wg_run_command( command );
				#else
						snprintf( command, sizeof( command ),
								"mv "
								"-f \"%s\" \"%s/%s/\"",
								wgconfig.wg_sef_found_list[i], dump_loc, dump_place );

						wg_run_command( command );
				#endif
						pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s - %s\n",
								 wgconfig.wg_sef_found_list[i], dump_loc, dump_place );
				} else {/* Move to current directory */
						if ( rate_has_prefix ) {
						#ifdef WG_WINDOWS
								snprintf( command, sizeof( command ),
										"move "
										"/Y \"%s\" \"%s\"",
										wgconfig.wg_sef_found_list[i], dump_loc );

								wg_run_command( command );
						#else
								snprintf( command, sizeof( command ),
										"mv "
										"-f \"%s\" \"%s\"",
										wgconfig.wg_sef_found_list[i], dump_loc );

								wg_run_command( command );
						#endif
								pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\n",
												wgconfig.wg_sef_found_list[i], dump_loc );
						} else {
								if ( path_exists( "plugins" ) == 1 ) {
								#ifdef WG_WINDOWS
										snprintf( command, sizeof( command ),
												"move "
												"/Y \"%s\" \"%s\\plugins\"",
												wgconfig.wg_sef_found_list[i], dump_loc );

										wg_run_command( command );

										pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\\plugins\n",
														wgconfig.wg_sef_found_list[i], dump_loc );
								#else
										snprintf( command, sizeof( command ),
												"mv "
												"-f \"%s\" \"%s/plugins\"",
												wgconfig.wg_sef_found_list[i], dump_loc );

										wg_run_command( command );

										pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s/plugins\n",
														wgconfig.wg_sef_found_list[i], dump_loc );
								#endif
								}
						}

						if ( rate_has_prefix ) {
								pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\n",
												wgconfig.wg_sef_found_list[i], dump_loc );
						} else {
								if ( path_exists( "plugins" ) == 1 ) {
								#ifdef WG_WINDOWS
										pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s\\plugins\n",
												wgconfig.wg_sef_found_list[i], dump_loc );
								#else
										pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Plugins %s -> %s/plugins\n",
												wgconfig.wg_sef_found_list[i], dump_loc );
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
void package_move_files( const char *package_dir, const char *package_loc )
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
		int rate_found_include = REPLICATE_RATE_ZERO;

		/* Reset */
		fdir_counts = REPLICATE_RATE_ZERO;

		/* Stack-based directory traversal for finding nested include files */
		struct stat dir_st;
		struct dirent *dir_item;
		char **tmp_stack = wg_malloc( _ssize * sizeof( char* ) );
		if ( tmp_stack == REPLICATE_RATE_ZERO ) { return; }
		char **stack = tmp_stack;
		for ( i = REPLICATE_RATE_ZERO; i < _ssize; i++ ) {
		    stack[i] = wg_malloc( WG_PATH_MAX );
		    if ( stack[i] == REPLICATE_RATE_ZERO ) {
		        for ( int j = REPLICATE_RATE_ZERO; j < i; j++ ) {
		            wg_free( stack[j] );
		        }
		        wg_free( stack );
		        return;
		    }
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
				for ( i = REPLICATE_RATE_ZERO; i < wgconfig.wg_sef_count; ++i ) {
						packages = PACKAGE_GET_FILENAME( wgconfig.wg_sef_found_list[i] );
				#ifdef WG_WINDOWS
						snprintf( command, sizeof( command ),
								"move "
								"/Y \"%s\" \"%s\\%s\\\"",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );

						wg_run_command( command );

						pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );
				#else
						snprintf( command, sizeof( command ),
								"mv "
								"-f \"%s\" \"%s/%s/\"",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );

						wg_run_command( command );

						pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );
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
				for ( i = REPLICATE_RATE_ZERO; i < wgconfig.wg_sef_count; ++i ) {
						const char *packages;
						packages = PACKAGE_GET_FILENAME( wgconfig.wg_sef_found_list[i] );

				#ifdef WG_WINDOWS
						snprintf( command, sizeof( command ),
								"move "
								"/Y \"%s\" \"%s\\%s\\\"",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );

						wg_run_command( command );

						pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s\\? -> %s - %s\\?\n",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );
				#else
						snprintf( command, sizeof( command ),
								"mv "
								"-f \"%s\" \"%s/%s/\"",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );

						wg_run_command( command );

						pr_color( stdout, FCOLOUR_CYAN, " [REPLICATE] Include %s/? -> %s - %s/?\n",
								wgconfig.wg_sef_found_list[i], package_loc, include_path );
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
		
		char *size_location = strdup(package_loc);
		/* Move plugin files */
#ifndef WG_WINDOWS
		dump_file_type( plugins, "*.dll", NULL, size_location, "/plugins", 0 );
		dump_file_type( plugins, "*.so", NULL, size_location, "/plugins", 0 );
#else
		dump_file_type( plugins, "*.dll", NULL, size_location, "\\plugins", 0 );
		dump_file_type( plugins, "*.so", NULL, size_location, "\\plugins", 0 );
#endif

#if defined(_DBG_PRINT)
		println(stdout, "plug path: %s", plugins);
#endif
		/* Move root-level plugin files */
		dump_file_type( package_dir, "*.dll", "plugins", size_location, "", 1 );
		dump_file_type( package_dir, "*.so", "plugins", size_location, "", 1 );

		/* Handle open.mp specific components */
		if ( wg_server_env() == 2 ) {
		#ifdef WG_WINDOWS
				snprintf( includes, sizeof( includes ), "qawno\\include" );
		#else
				snprintf( includes, sizeof( includes ), "qawno/include" );
		#endif
				dump_file_type( components, "*.dll", NULL, size_location, "components", 0 );
				dump_file_type( components, "*.so", NULL, size_location, "components", 0 );
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
		while ( _sindex >= REPLICATE_RATE_ZERO ) {
				strlcpy( root_dir, stack[_sindex], sizeof( root_dir ) );
				--_sindex;

				DIR *open_dir = opendir( root_dir );
				if ( !open_dir )
						{
								continue;
						}

				while ( ( dir_item = readdir( open_dir ) ) != NULL ) {
						if ( wg_dot_or_dotdot( dir_item->d_name ) )
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

						if ( stat( the_path, &dir_st ) != REPLICATE_RATE_ZERO )
								{
										continue;
								}

						if ( S_ISDIR( dir_st.st_mode ) ) {
								/* Skip compiler directories */
								if ( strcmp( dir_item->d_name, "pawno" ) == REPLICATE_RATE_ZERO ||
								    strcmp( dir_item->d_name, "qawno" ) == REPLICATE_RATE_ZERO ||
								    strcmp( dir_item->d_name, "include" ) == REPLICATE_RATE_ZERO ||
								    strcmp ( dir_item->d_name, "components") == REPLICATE_RATE_ZERO ||
								    strcmp ( dir_item->d_name, "plugins") == REPLICATE_RATE_ZERO)
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
						    strcmp( strrchr( dir_item->d_name, '.' ), ".inc" ) == REPLICATE_RATE_ZERO )
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
								for ( j = REPLICATE_RATE_ZERO;
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
								wg_free(size_src);
						#else
								/* Unix: recursive copy and remove */
								const char *unix_move_parts[] = {
										"cp -r ", src, " ", dest, " && rm -rf ", src
								};
								size_t size_unix_move_parts = sizeof( unix_move_parts ),
								       size_unix_move_parts_zero = sizeof( unix_move_parts[0] );
								strlcpy( command, "", sizeof( command ) );
								int j;
								for ( j = REPLICATE_RATE_ZERO;
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
		for ( i = REPLICATE_RATE_ZERO;
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
void wg_apply_depends( const char *depends_name, const char *depends_location )
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

		char size_depends_location[WG_PATH_MAX * 2];

		/* Create necessary directories based on server environment */
		if ( wg_server_env() == 1 ) {
				snprintf(size_depends_location, sizeof(size_depends_location),
						"%s/pawno/include", depends_location);
				if ( dir_exists( size_depends_location ) == REPLICATE_RATE_ZERO ) {
						wg_mkdir( size_depends_location );
				}
				snprintf(size_depends_location, sizeof(size_depends_location),
						"%s/plugins", depends_location);
				if ( dir_exists( size_depends_location ) == REPLICATE_RATE_ZERO ) {
						wg_mkdir( size_depends_location );
				}
		} else if ( wg_server_env() == 2 ) {
				snprintf(size_depends_location, sizeof(size_depends_location),
						"%s/qawno/include", depends_location);
				if ( dir_exists( size_depends_location ) == REPLICATE_RATE_ZERO ) {
						wg_mkdir( size_depends_location );
				}
				snprintf(size_depends_location, sizeof(size_depends_location),
						"%s/plugins", depends_location);
				if ( dir_exists( size_depends_location ) == REPLICATE_RATE_ZERO ) {
						wg_mkdir( size_depends_location );
				}
				snprintf(size_depends_location, sizeof(size_depends_location),
						"%s/components", depends_location);
				if ( dir_exists( size_depends_location ) == REPLICATE_RATE_ZERO ) {
						wg_mkdir( size_depends_location );
				}
		}

		/* Organize dependency files */
		package_move_files( package_dir, depends_location );
}

/*
 * Main dependency installation function
 * Parses dependency strings, downloads archives, and applies dependencies
 */
void wg_install_depends( const char *packages, const char *branch, const char *where )
{
		char buffer[1024];
		char package_url[1024];
		char *procure_buffer;
		struct _repositories repo;
		char package_name[WG_PATH_MAX];
		const char *size_last_slash;
		const char *dependencies[MAX_DEPENDS];
		int package_counts = REPLICATE_RATE_ZERO;
		int i;

		memset( dependencies, REPLICATE_RATE_ZERO, sizeof( dependencies ) );
		wgconfig.wg_idepends = REPLICATE_RATE_ZERO;

		/* Validate input */
		if ( !packages || !*packages )
				{
						pr_color( stdout, FCOLOUR_RED, "" );
						printf( "no valid dependencies to install!"
								"\t\t[Fail]\n" );
						goto done;
				}

		if ( strlen( packages ) >= sizeof( buffer ) )
				{
						pr_color( stdout, FCOLOUR_RED, "" );
						printf( "packages too long!"
								"\t\t[Fail]\n" );
						goto done;
				}

		/* Tokenize dependency string */
		snprintf( buffer, sizeof( buffer ), "%s", packages );

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
		for ( i = REPLICATE_RATE_ZERO; i < package_counts; i++ ) {
				/* Parse repository information */
				if ( !package_parse_repo( dependencies[i], &repo ) )
						{
								pr_color( stdout, FCOLOUR_RED, "" );
								printf( "invalid repo format: %s"
										"\t\t[Fail]\n", dependencies[i] );
								continue;
						}

				/* Handle GitHub _repositories */
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
						/* Handle custom _repositories */
						package_build_repo_url( &repo, REPLICATE_RATE_ZERO, package_url, sizeof( package_url ) );
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
				if (where == NULL || where[0] == '\0')
					{
						static int k = 0;
						static char *__location = NULL;
						if (! k ) {
							printf("\n");
							println(stdout, FCOLOUR_YELLOW "==== Location is Null: %s ====", where);
							printf(">>> where you want? just enter if you want install in root...\n");
							printf("   example: " FCOLOUR_CYAN "../storage/downloads/myproj" FCOLOUR_DEFAULT "\n");
							printf("            " FCOLOUR_CYAN "myfolder/myproj" FCOLOUR_DEFAULT "\n");
							fflush(stdout);
							char *locations = readline("> ");
							if (!locations || locations[0] == '.') {
								static char *fetch_pwd = NULL;
								fetch_pwd = wg_procure_pwd();
								__location = strdup(fetch_pwd);
								wg_apply_depends( package_name, fetch_pwd );
							} else {
								if (dir_exists(locations) == 0) {
									wg_mkdir(locations);
								}
								__location = strdup(locations);
								wg_apply_depends ( package_name, locations );
							}
							++k;
						} else {
							wg_apply_depends ( package_name, __location );
						}
					}
				else {
					if (dir_exists(where) == 0) {
						wg_mkdir(where);
					}
					wg_apply_depends ( package_name, where );
				}
		}

done:
		return;
}
