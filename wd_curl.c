#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_crypto.h"
#include "wd_archive.h"
#include "wd_depends.h"
#include "wd_util.h"
#include "wd_unit.h"
#include "wd_curl.h"

// This is a string variable used to statically store the PawnCC folder name,
// ensuring that PawnCC is applied from the correct directory and not from an incorrect extraction folder.
static char
	pawncc_dir_src[WD_PATH_MAX];

void verify_cacert_pem(CURL *curl) {
		int is_win32 = 0;
#ifdef WD_WINDOWS
		is_win32 = 1;
#endif
		int is_android = 0;
#ifdef WD_ANDROID
		is_android = 2;
#endif
		int is_linux = 0;
#ifdef WD_LINUX
		is_linux = 3;
#endif
		if (is_win32) {
				if (access("cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
				else if (access("C:/libwatchdogs/cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
				else {
					pr_color(stdout,
							 FCOLOUR_YELLOW,
							 "Warning: No CA file found. SSL verification may fail.\n");
				}
		} else if (is_android) {
				const char *env_home = getenv("HOME");
				char size_env_home[WD_PATH_MAX + 6];
				wd_snprintf(size_env_home, sizeof(size_env_home), "%s/cacert.pem", env_home);
				if (access("cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
				else if (access(size_env_home, F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, size_env_home);
				else {
					pr_color(stdout,
							 FCOLOUR_YELLOW,
							 "Warning: No CA file found. SSL verification may fail.\n");
				}
		} else if (is_linux) {
				if (access("cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
				else if (access("/etc/ssl/certs/cacert.pem", F_OK) == 0)
					curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/cacert.pem");
				else {
					pr_color(stdout,
							 FCOLOUR_YELLOW,
							 "Warning: No CA file found. SSL verification may fail.\n");
				}
		}
}

/**
 * Progress callback function for CURL downloads
 * Displays download progress with percentage and visual indicator
 * 
 * @param ptr User-defined pointer (not used)
 * @param dltotal Total bytes to download
 * @param dlnow Bytes downloaded so far
 * @param ultotal Total bytes to upload (not used)
 * @param ulnow Bytes uploaded so far (not used)
 * @return Always returns __RETZ (success)
 */
static int progress_callback(void *ptr, curl_off_t dltotal,
						     curl_off_t dlnow, curl_off_t ultotal,
						     curl_off_t ulnow)
{
		static int last_percent = -1;  /* Store last displayed percentage to avoid flickering */
		int percent;                   /* Current download percentage */

		/* Only calculate percentage if we know the total download size */
		if (dltotal > 0) {
				/* Calculate current download percentage */
				percent = (int)((dlnow * 100) / dltotal);
				/* Only update display if percentage has changed */
				if (percent != last_percent) {
						/* Display different message for incomplete vs complete downloads */
						if (percent < 100)
								printf("\r\tDownloading... %3d%% ?-", percent);  /* In progress */
						else
								printf("\r\tDownloading... %3d%% - ", percent);   /* Complete */

						fflush(stdout);      /* Ensure output is displayed immediately */
						last_percent = percent;  /* Update last displayed percentage */
				}
		}

		return __RETZ;  /* Always return success to continue download */
}

/**
 * Memory write callback for CURL
 * Stores downloaded data in a dynamically growing memory buffer
 * 
 * @param contents Pointer to the data received from CURL
 * @param size Size of each data element
 * @param nmemb Number of data elements
 * @param userp User pointer to memory_struct
 * @return Number of bytes actually processed
 */
size_t write_memory_callback(void *contents, size_t size,
							size_t nmemb, void *userp)
{
		struct memory_struct *mem = userp;  /* Cast user pointer to memory structure */
		size_t realsize = size * nmemb;     /* Calculate actual data size received */
		char *ptr;                          /* Temporary pointer for realloc */

		/* Validate input parameters */
		if (!contents || !mem)
			return __RETZ;

		/* Reallocate memory buffer to accommodate new data plus null terminator */
		ptr = wd_realloc(mem->memory, mem->size + realsize + 1);
		if (!ptr) {
#if defined(_DBG_PRINT)
			/* Only print memory error in debug mode */
			pr_error(stdout,
				   "Out of memory detected in %s\n", __func__);
#endif
			return __RETZ;
		}

		/* Update memory structure with new buffer and copy data */
		mem->memory = ptr;
		memcpy(&mem->memory[mem->size], contents, realsize);
		mem->size += realsize;           /* Update total size */
		mem->memory[mem->size] = '\0';   /* Null-terminate the buffer */

		return realsize;  /* Return number of bytes processed */
}

/**
 * Extracts archive files based on their type
 * Supports TAR, TAR.GZ, and ZIP formats
 * 
 * @param filename Path to the archive file to extract
 * @return 0 on success, negative error code on failure
 */
static int extract_archive(const char *filename)
{
		char output_path[WD_PATH_MAX];  /* Buffer for extraction path */
		size_t name_len;                /* Length of filename */

		/* Check for TAR archive formats */
		if (strstr(filename, ".tar") || strstr(filename, ".tar.gz")) {
				printf(" Try Extracting TAR archive: %s\n", filename);
				return wd_extract_tar(filename);  /* Extract using TAR handler */
		} else if (strstr(filename, ".zip")) {
				/* Handle ZIP archive extraction */
				printf(" Try Extracting ZIP archive: %s\n", filename);

				/* Remove .zip extension for output directory */
				name_len = strlen(filename);
				if (name_len > 4 &&
					!strncmp(filename + name_len - 4, ".zip", 4))
				{
						/* Copy filename without .zip extension */
						wd_strncpy(output_path, filename, name_len - 4);
						output_path[name_len - 4] = '\0';
				} else {
						/* Use full filename if no .zip extension found */
						wd_strcpy(output_path, filename);
				}

				return wd_extract_zip(filename, output_path);  /* Extract using ZIP handler */
		} else {
				/* Unknown archive format - log and return error */
				pr_info(stdout, "Unknown archive type, skipping extraction: %s\n", filename);
				return -__RETN;
		}
}

/**
 * Searches for compiler tools in the specified directory
 * Identifies pawn compiler components for different platforms
 * 
 * @param compiler_type Type of compiler (SAMP or OPENMP)
 * @param found_pawncc_exe Output: found pawncc.exe status
 * @param found_pawncc Output: found pawncc (Unix) status
 * @param found_pawndisasm_exe Output: found pawndisasm.exe status
 * @param found_pawndisasm Output: found pawndisasm (Unix) status
 * @param found_pawnc_dll Output: found pawnc.dll status
 * @param found_PAWNC_DLL Output: found PAWNC.dll status
 */
static void find_compiler_tools(int compiler_type,
								int *found_pawncc_exe, int *found_pawncc,
								int *found_pawndisasm_exe, int *found_pawndisasm,
								int *found_pawnc_dll, int *found_PAWNC_DLL)
{
		const char *ignore_dir = NULL;  /* No directories to ignore in search */
		char size_pf[WD_PATH_MAX + 56]; /* Buffer for search path */

		/* Build search path in bin directory */
		wd_snprintf(size_pf, sizeof(size_pf), "%s/bin", pawncc_dir_src);
		
		/* Search for each compiler tool in the bin directory */
		*found_pawncc_exe = wd_sef_fdir(size_pf, "pawncc.exe", ignore_dir);
		*found_pawncc = wd_sef_fdir(size_pf, "pawncc", ignore_dir);
		*found_pawndisasm_exe = wd_sef_fdir(size_pf, "pawndisasm.exe", ignore_dir);
		*found_pawndisasm = wd_sef_fdir(size_pf, "pawndisasm", ignore_dir);
		*found_pawnc_dll = wd_sef_fdir(size_pf, "pawnc.dll", ignore_dir);
		*found_PAWNC_DLL = wd_sef_fdir(size_pf, "PAWNC.dll", ignore_dir);
}

/**
 * Determines and creates the appropriate compiler directory
 * Checks for existing pawno/qawno directories or creates new one
 * 
 * @return Path to compiler directory, or NULL on failure
 */
static const char *get_compiler_directory(void)
{
		struct stat st;           /* File status structure for directory checks */
		const char *dir_path = NULL;  /* Selected directory path */

		/* Check for existing pawno directory */
		if (stat("pawno", &st) == 0 && S_ISDIR(st.st_mode)) {
				dir_path = "pawno";
		} else if (stat("qawno", &st) == 0 && S_ISDIR(st.st_mode)) {
				/* Check for existing qawno directory */
				dir_path = "qawno";
		} else {
				/* Create new pawno directory if none exist */
				if (MKDIR("pawno") == 0)
						dir_path = "pawno";
		}
		
		/* Create include directories if they don't exist */
		if (stat("pawno/include", &st) != 0 && errno == ENOENT) {
			mkdir_recusrs("pawno/include");  /* Create pawno/include recursively */
		}
		else if (stat("qawno/include", &st) != 0 && errno == ENOENT) {
			mkdir_recusrs("qawno/include");  /* Create qawno/include recursively */
		}

		return dir_path;  /* Return selected directory path */
}

/**
 * Copies compiler tools from source to destination directory
 * 
 * @param src_path Source path of the tool
 * @param tool_name Name of the tool file
 * @param dest_dir Destination directory
 */
static void copy_compiler_tool(const char *src_path, const char *tool_name,
						       const char *dest_dir)
{
		char dest_path[WD_PATH_MAX];  /* Buffer for destination path */

		/* Build full destination path and move the file */
		wd_snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
		wd_sef_wmv(src_path, dest_path);  /* Move file to destination */
}

#ifndef WD_WINDOWS
/**
 * Updates library environment variables for Linux/Unix systems
 * Handles library path configuration and system cache updates
 * 
 * @param lib_path Library path to add to environment
 */
static void update_library_environment(const char *lib_path)
{
		const char *old_path;     /* Existing LD_LIBRARY_PATH value */
		char new_path[256];       /* New LD_LIBRARY_PATH value */

		/* Update system library cache if path is in system directory */
		if (strstr(lib_path, "/usr/local")) {
				/* Try to use sudo for system-wide update, fall back to regular ldconfig */
				int is_not_sudo = wd_run_command("which sudo > /dev/null 2>&1");
				if (is_not_sudo == 0)
						wd_run_command("sudo ldconfig");  /* Update with sudo */
				else
						wd_run_command("ldconfig");       /* Update without sudo */
		}

		/* Get current LD_LIBRARY_PATH environment variable */
		old_path = getenv("LD_LIBRARY_PATH");
		if (old_path) {
				/* Prepend new path to existing paths */
				wd_snprintf(new_path, sizeof(new_path), "%s:%s", lib_path, old_path);
		} else {
				/* Use new path as the only library path */
				wd_snprintf(new_path, sizeof(new_path), "%s", lib_path);
		}

		/* Set updated library path in environment */
		SETENV("LD_LIBRARY_PATH", new_path, 1);
}
#endif

/**
 * Sets up Linux library installation for pawn compiler
 * Copies library files to system directories and updates environment
 * 
 * @return __RETZ on success, -__RETN on failure
 */
static int setup_linux_library(void)
{
#ifdef WD_WINDOWS
		/* Skip library setup on Windows systems */
		return __RETZ;
#else
		const char *selected_path = NULL;  /* Chosen library installation path */
		char libpawnc_src[WD_PATH_MAX];    /* Source path of library file */
		char dest_path[WD_PATH_MAX];       /* Destination path for library */
		struct stat st;                    /* File status structure */
		int i, found_lib;                  /* Loop counter and library found flag */
		
		/* Possible library installation paths for different environments */
		const char *lib_paths[] =	{
										"/data/data/com.termux/files/usr/lib/",      /* Termux ARM */
										"/data/data/com.termux/files/usr/local/lib/", /* Termux local */
										"/data/data/com.termux/arm64/usr/lib",        /* Termux ARM64 */
										"/data/data/com.termux/arm32/usr/lib",        /* Termux ARM32 */
										"/data/data/com.termux/amd32/usr/lib",        /* Termux x86 */
										"/data/data/com.termux/amd64/usr/lib",        /* Termux x64 */
										"/usr/local/lib",                            /* System local lib */
										"/usr/local/lib32"                           /* System local lib32 */
									};

		/* Skip setup on Windows or unknown OS types */
		if (!strcmp(wcfg.wd_toml_os_type, OS_SIGNAL_WINDOWS) ||
			!strcmp(wcfg.wd_toml_os_type, OS_SIGNAL_UNKNOWN))
				return __RETZ;

		/* Search for libpawnc.so in the lib directory */
		char size_pf[WD_PATH_MAX + 56];
		wd_snprintf(size_pf, sizeof(size_pf), "%s/lib", pawncc_dir_src);
		found_lib = wd_sef_fdir(size_pf, "libpawnc.so", NULL);
		if (!found_lib)
				return __RETZ;  /* Return if library not found */

		/* Find the actual path of the library file */
		for (i = 0; i < wcfg.wd_sef_count; i++) {
				if (strstr(wcfg.wd_sef_found_list[i], "libpawnc.so")) {
						wd_strncpy(libpawnc_src, wcfg.wd_sef_found_list[i], sizeof(libpawnc_src));
						break;
				}
		}

		/* Find a valid library installation path */
		for (i = 0; i < sizeof(lib_paths) / sizeof(lib_paths[0]); i++) {
				if (stat(lib_paths[i], &st) == 0 && S_ISDIR(st.st_mode)) {
						selected_path = lib_paths[i];  /* Use first valid path found */
						break;
				}
		}

		/* Error if no valid library path found */
		if (!selected_path) {
				fprintf(stderr, "No valid library path found!\n");
				return -__RETN;
		}

		/* Copy library to system directory */
		wd_snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
		wd_sef_wmv(libpawnc_src, dest_path);

		/* Update library environment variables */
		update_library_environment(selected_path);

		return __RETZ;  /* Return success */
#endif
}

/**
 * Applies pawn compiler tools to the project
 * Copies compiler binaries and libraries to appropriate directories
 */
void wd_apply_pawncc(void)
{
		/* Flags indicating which compiler tools were found */
		int found_pawncc_exe, found_pawncc;
		int found_pawndisasm_exe, found_pawndisasm;
		int found_pawnc_dll, found_PAWNC_DLL;
		
		const char *dest_dir;  /* Destination directory for compiler tools */
		
		/* Buffers for source paths of each compiler tool */
		char pawncc_src[WD_PATH_MAX] = { 0 },
			 pawncc_exe_src[WD_PATH_MAX] = { 0 },
			 pawndisasm_src[WD_PATH_MAX] = { 0 },
			 pawndisasm_exe_src[WD_PATH_MAX] = { 0 },
			 pawnc_dll_src[WD_PATH_MAX] = { 0 },
			 PAWNC_DLL_src[WD_PATH_MAX] = { 0 };

		int i;  /* Loop counter */

		/* Reset file directory search results */
		wd_sef_fdir_reset();

		/* Search for compiler tools based on server type */
		find_compiler_tools(wcfg.wd_is_samp ? COMPILER_SAMP : COMPILER_OPENMP,
						    &found_pawncc_exe, &found_pawncc,
						    &found_pawndisasm_exe, &found_pawndisasm,
							&found_pawnc_dll, &found_PAWNC_DLL);

		/* Get or create compiler directory */
		dest_dir = get_compiler_directory();
		if (!dest_dir) {
				pr_error(stdout, "Failed to create compiler directory");
				goto done;  /* Exit on directory creation failure */
		}

		/* Process search results to extract full paths of found tools */
		for (i = 0; i < wcfg.wd_sef_count; i++) {
				const char *item = wcfg.wd_sef_found_list[i];
				if (!item)
						continue;

				/* Identify and store paths for each compiler tool */
				if (strstr(item, "pawncc.exe")) {
						wd_strncpy(pawncc_exe_src, item, sizeof(pawncc_exe_src));
				} else if (strstr(item, "pawncc") && !strstr(item, ".exe")) {
						wd_strncpy(pawncc_src, item, sizeof(pawncc_src));
				} else if (strstr(item, "pawndisasm.exe")) {
						wd_strncpy(pawndisasm_exe_src, item, sizeof(pawndisasm_exe_src));
				} else if (strstr(item, "pawndisasm") && !strstr(item, ".exe")) {
						wd_strncpy(pawndisasm_src, item, sizeof(pawndisasm_src));
				} else if (strstr(item, "pawnc.dll"))  {
						wd_strncpy(pawnc_dll_src, item, sizeof(pawnc_dll_src));
				} else if (strstr(item, "PAWNC.dll")) {
						wd_strncpy(PAWNC_DLL_src, item, sizeof(PAWNC_DLL_src));
				}
		}

		/* Copy each found tool to the compiler directory */
		if (found_pawncc_exe && pawncc_exe_src[0])
				copy_compiler_tool(pawncc_exe_src, "pawncc.exe", dest_dir);

		if (found_pawncc && pawncc_src[0])
				copy_compiler_tool(pawncc_src, "pawncc", dest_dir);

		if (found_pawndisasm_exe && pawndisasm_exe_src[0])
				copy_compiler_tool(pawndisasm_exe_src, "pawndisasm.exe", dest_dir);

		if (found_pawndisasm && pawndisasm_src[0])
				copy_compiler_tool(pawndisasm_src, "pawndisasm", dest_dir);

		if (found_pawnc_dll && pawnc_dll_src[0])
				copy_compiler_tool(pawnc_dll_src, "pawnc.dll", dest_dir);

		if (found_PAWNC_DLL && PAWNC_DLL_src[0])
				copy_compiler_tool(PAWNC_DLL_src, "PAWNC.dll", dest_dir);

		/* Setup Linux library environment if applicable */
		setup_linux_library();

		/* Clean up downloaded compiler source directory */
		char rm_cmd[WD_PATH_MAX + 56];
		if (is_native_windows())
			/* Windows delete command with error suppression */
			wd_snprintf(rm_cmd, sizeof(rm_cmd),
				"rmdir /s /q \"%s\"",
				pawncc_dir_src);
		else
			/* Unix/Linux recursive delete */
			wd_snprintf(rm_cmd, sizeof(rm_cmd),
				"rm -rf %s",
				pawncc_dir_src);
		wd_run_command(rm_cmd);

		/* Clear the source directory variable */
		memset(pawncc_dir_src, 0, sizeof(pawncc_dir_src));

		/* Display success message */
		pr_info(stdout, "Compiler installed successfully!");

/* Cleanup label for error handling */
done:
		/* Return to main menu */
		wd_main(NULL);
}

/**
 * Prompts user to apply pawn compiler after download
 * 
 * @return __RETN if user confirms, __RETZ if user declines
 */
static int prompt_apply_pawncc(void)
{
		wcfg.wd_ipawncc = 0;  /* Reset pawncc installation flag */

		/* Display prompt in yellow color */
		pr_color(stdout, FCOLOUR_YELLOW, "Apply pawncc?");
		char *confirm = readline(" [y/n]: ");
		fflush(stdout);
		
		/* Handle input reading errors */
		if (!confirm) {
			fprintf(stderr, "Error reading input\n");
			wd_free(confirm);
			goto done;
		}
		
		/* Handle empty input by re-prompting */
		if (strlen(confirm) == 0) {
			wd_free(confirm);
			confirm = readline(" >>> [y/n]: ");
		}
		
		/* Process user confirmation */
		if (confirm) {
			if (strcmp(confirm, "Y") == 0 || strcmp(confirm, "y") == 0) {
				wd_free(confirm);
				return __RETN;  /* User confirmed - apply pawncc */
			}
		}

/* Cleanup label */
done:
		return __RETZ;  /* User declined or error occurred */
}

/**
 * Checks if a file is an archive based on its extension
 * 
 * @param filename File name to check
 * @return 1 if file is an archive, 0 otherwise
 */
int is_archive_file(const char *filename)
{
		const char *ext = strrchr(filename, '.');  /* Find file extension */
		if (!ext) return 0;  /* No extension - not an archive */

		/* List of supported archive extensions */
		const char *archive_exts[] = {
										".zip",      /* ZIP archive */
										".tar",      /* TAR archive */
										".tar.gz",   /* Compressed TAR archive */
										NULL         /* End of list marker */
									 };

		/* Check if file extension matches any known archive type */
		for (int i = 0; archive_exts[i] != NULL; i++) {
			if (strcasecmp(ext, archive_exts[i]) == 0) {
				return 1;  /* File is an archive */
			}
		}

		return 0;  /* File is not a recognized archive */
}

/**
 * Downloads a file from URL with retry logic and progress display
 * Handles archive extraction and compiler installation if applicable
 * 
 * @param url URL to download from
 * @param filename Local filename to save as
 * @return __RETZ on success, -__RETN on failure
 */
int wd_download_file(const char *url, const char *filename)
{
		CURL *curl;                    /* CURL handle for HTTP requests */
		FILE *c_fp;                    /* File pointer for writing download */
		CURLcode res;                  /* CURL operation result */
		long response_code = 0;        /* HTTP response code */
		int retry_count = 0;           /* Counter for retry attempts */
		struct stat file_stat;         /* File status for size verification */

		/* Display download initiation message */
		pr_color(stdout, FCOLOUR_GREEN, "Downloading: %s\n", filename);

		/* Retry loop for download attempts */
		do {
			/* Open file for binary writing */
			c_fp = fopen(filename, "wb");
			if (!c_fp) {
				pr_color(stdout, FCOLOUR_RED, "Failed to open file: %s\n", filename);
				return -__RETN;
			}

			/* Initialize CURL session */
			curl = curl_easy_init();
			if (!curl) {
				pr_color(stdout, FCOLOUR_RED, "Failed to initialize CURL\n");
				fclose(c_fp);
				return -__RETN;
			}

			/* Prepare HTTP headers */
			struct curl_slist *headers = NULL;

			/* Add GitHub authentication if downloading dependencies */
			if (wcfg.wd_idepends == 1) {
				if (strstr(wcfg.wd_toml_github_tokens, "DO_HERE")) {
						pr_color(stdout, FCOLOUR_GREEN, "\tcan't read github token...\n");
						sleep(1);
				} else {
						/* Create Authorization header with GitHub token */
						char auth_header[512];
						wd_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", wcfg.wd_toml_github_tokens);
						headers = curl_slist_append(headers, auth_header);
						pr_color(stdout,
								 FCOLOUR_GREEN,
								 "\tCreate token: %s...\n",
								 wd_masked_text(8,
												 wcfg.wd_toml_github_tokens));
				}
			}

			/* Add standard HTTP headers */
			headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
			headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

			/* Configure CURL options */
			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, c_fp);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

			printf("\n");
			/* Set progress callback for download tracking */
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

			/* Verify SSL certificates */
			verify_cacert_pem(curl);

			fflush(stdout);
			/* Perform the download */
			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			/* Clean up CURL resources */
			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);

			/* Close file */
			if (c_fp)
				fclose(c_fp);

			/* Check if download was successful */
			if (res == CURLE_OK && response_code == 200) {
				/* Verify downloaded file exists and has content */
				if (stat(filename, &file_stat) == 0 && file_stat.st_size > 0) {
					printf("Download successful: %" PRIdMAX " bytes\n", (intmax_t)file_stat.st_size);
					fflush(stdout);

					/* Check if downloaded file is an archive */
					if (is_archive_file(filename)) {
						printf("Checking file type for extraction...\n");
						fflush(stdout);
						/* Extract the archive */
						extract_archive(filename);

						char rm_cmd[WD_PATH_MAX];
						/* Handle archive cleanup based on dependency type */
						if (wcfg.wd_idepends == 1) {
							static int rm_deps = 0;
							if (rm_deps == 0) {
								/* Prompt user to remove dependency archive */
								pr_color(stdout, FCOLOUR_GREEN, "* remove archive '%s'? ", filename);
								fflush(stdout);
								char *confirm = readline(" [y/n]: ");
								if (confirm) {
									if (confirm[0] == 'Y' || confirm[0] == 'y') {
										rm_deps = 1;
										/* Build platform-specific delete command */
										if (is_native_windows())
											wd_snprintf(rm_cmd, sizeof(rm_cmd),
												"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
												"rmdir /s /q \"%s\" 2>nul)",
												filename, filename, filename);
										else
											wd_snprintf(rm_cmd, sizeof(rm_cmd),
												"rm -rf %s",
												filename);
										wd_run_command(rm_cmd);
									}
									wd_free(confirm);
								}
							} else {
								/* Auto-remove if user previously confirmed */
								if (is_native_windows())
									wd_snprintf(rm_cmd, sizeof(rm_cmd),
										"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
										"rmdir /s /q \"%s\" 2>nul)",
										filename, filename, filename);
								else
									wd_snprintf(rm_cmd, sizeof(rm_cmd),
										"rm -rf %s",
										filename);
								wd_run_command(rm_cmd);
							}
						} else {
							/* Prompt for regular file removal */
							pr_color(stdout, FCOLOUR_GREEN, "remove archive '%s'? ", filename);
							fflush(stdout);
							char *confirm = readline(" [y/n]: ");
							if (confirm) {
								if (confirm[0] == 'Y' || confirm[0] == 'y') {
									char rm_cmd[WD_PATH_MAX];
									/* Build platform-specific delete command */
									if (is_native_windows())
										wd_snprintf(rm_cmd, sizeof(rm_cmd),
											"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
											"rmdir /s /q \"%s\" 2>nul)",
											filename, filename, filename);
									else
										wd_snprintf(rm_cmd, sizeof(rm_cmd),
											"rm -rf %s",
											filename);
									wd_run_command(rm_cmd);
								}
								wd_free(confirm);
							}
						}
					} else {
						/* File is not an archive - skip extraction */
						printf("File is not an archive, skipping extraction.\n");
						fflush(stdout);
					}

					/* Check if pawncc installation is requested */
					if (wcfg.wd_ipawncc && prompt_apply_pawncc()) {
						/* Prepare source directory for pawncc installation */
						char size_filename[WD_PATH_MAX];
						wd_snprintf(size_filename, sizeof(size_filename), "%s", filename);
						char *f_EXT = strrchr(size_filename, '.');
						if (f_EXT)
								*f_EXT = '\0';  /* Remove extension */
						wd_snprintf(pawncc_dir_src, sizeof(pawncc_dir_src), "%s", size_filename);
						/* Apply pawncc compiler tools */
						wd_apply_pawncc();
					}

					return __RETZ;  /* Return success */
				} else {
					/* Downloaded file is empty or missing */
					pr_color(stdout, FCOLOUR_RED, "Downloaded file is empty or missing\n");
					fflush(stdout);
				}
			} else {
				/* Download failed - display error details */
				pr_color(stdout, FCOLOUR_RED, "Download failed - "
						 "HTTP: %ld, "
						 "CURL: %d, "
						 "retrying...\n", response_code, res);
			}

			/* Increment retry counter and wait before retry */
			++retry_count;
			sleep(3);
		} while (retry_count < 5);  /* Maximum 5 retry attempts */

		/* All retry attempts failed */
		pr_color(stdout, FCOLOUR_RED, "Download failed after 5 retries\n");
		wd_main(NULL);
		return 0;
}