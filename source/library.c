#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include "extra.h"
#include "utils.h"

#ifndef WG_WINDOWS
	#include <sys/utsname.h> /* System information header for Unix-like systems */
#else
    #include <windows.h>
    #include <iphlpapi.h>
    #include <intrin.h>
struct utsname {
        char sysname[WG_PATH_MAX];
        char nodename[WG_PATH_MAX];
        char release[WG_PATH_MAX];
        char version[WG_PATH_MAX];
        char machine[WG_PATH_MAX];
};
int uname(struct utsname *name)
{
        OSVERSIONINFOEX osvi;
        SYSTEM_INFO si;

        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

        if (!GetVersionEx((OSVERSIONINFO*)&osvi)) {
                return -1;
        }

        GetSystemInfo(&si);

        snprintf(name->sysname, sizeof(name->sysname), "Windows");
        snprintf(name->release, sizeof(name->release), "%lu.%lu",
                                osvi.dwMajorVersion, osvi.dwMinorVersion);
        snprintf(name->version, sizeof(name->version), "Build %lu",
                                osvi.dwBuildNumber);

        switch (si.wProcessorArchitecture) {
                case PROCESSOR_ARCHITECTURE_AMD64:
                        snprintf(name->machine, sizeof(name->machine), "x86_64");
                        break;
                case PROCESSOR_ARCHITECTURE_INTEL:
                        snprintf(name->machine, sizeof(name->machine), "x86");
                        break;
                case PROCESSOR_ARCHITECTURE_ARM:
                        snprintf(name->machine, sizeof(name->machine), "ARM");
                        break;
                case PROCESSOR_ARCHITECTURE_ARM64:
                        snprintf(name->machine, sizeof(name->machine), "ARM64");
                        break;
                default:
                        snprintf(name->machine, sizeof(name->machine), "Unknown");
                        break;
        }

        return 0;
}
#endif

#include "units.h"
#include "archive.h"
#include "curl.h"
#include "debug.h"
#include "library.h"

/*
 * Handles Pawn compiler installation specifically for Termux (Android) environment
 * Provides interactive version and architecture selection for Termux-specific builds
 * Uses mxp96 repository which provides precompiled binaries for Termux ARM architectures
 */
static int pawncc_handle_termux_installation(void)
{
		int version_index; /* Index of selected version in versions array */
		char version_selection, architecture[16], /* User's selection and target architecture */
			 url[WG_PATH_MAX * 3], filename[128]; /* Download URL and local filename buffers */
		
		/* Available Pawn compiler versions for Termux from mxp96 repository */
		const char *library_termux_versions[] = { "3.10.11", "3.10.10" };
		size_t size_library_termux_versions = sizeof(library_termux_versions);
		size_t size_library_termux_versions_zero = sizeof(library_termux_versions[0]);
		size_t version_count = size_library_termux_versions / size_library_termux_versions_zero; /* Calculate number of versions */

		/* Verify we're actually in Termux environment before proceeding */
		if (!is_termux_env())
				pr_info(stdout, "Currently not in Termux!"); /* Informational message */

		char *__version__; /* Buffer for user's version selection input */

/* Label for retrying version selection on invalid input */
ret_pawncc:
        /* Display version selection menu with yellow colored header */
        printf("\033[1;33m== Select the PawnCC version to download ==\033[0m\n");
		/* List all available versions with letter options (A/a, B/b, etc.) */
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s (mxp96)\n", /* Show both uppercase and lowercase options */
				(int)('A' + i), /* Uppercase option letter */
				(int)('a' + i), /* Lowercase option letter */
				library_termux_versions[i]); /* Version number */
		}

		/* Get user input for version selection */
		__version__ = readline("==> ");
		version_selection = __version__[0]; /* Only consider first character of input */
		
		/* Convert selection character to array index with case-insensitive handling */
		if (version_selection >= 'A' &&
			version_selection < 'A' + (char)version_count) {
			version_index = version_selection - 'A'; /* Uppercase: A=0, B=1, etc. */
		} else if (version_selection >= 'a' &&
			version_selection < 'a' + (char)version_count) {
			version_index = version_selection - 'a'; /* Lowercase: a=0, b=1, etc. */
		} else {
			/* Invalid selection - show error with valid range and retry */
			pr_error(stdout, "Invalid version selection '%c'. "
 					 "Input must be A..%c or a..%c\n",
				version_selection,
				'A' + (int)version_count - 1, /* Last valid uppercase letter */
				'a' + (int)version_count - 1); /* Last valid lowercase letter */
			wg_free(__version__); /* Free invalid input */
			goto ret_pawncc; /* Jump back to selection prompt */
		}
		wg_free(__version__); /* Free valid input after processing */

		/* Architecture detection for Termux */
		struct utsname uname_data; /* Structure for system information */

		/* Get system information to auto-detect architecture */
		if (uname(&uname_data) != 0) {
				pr_error(stdout, "Failed to get system information");
				return -1; /* Return error if system info unavailable */
		}

		/* Map common ARM architecture names to Termux-specific identifiers */
		if (strcmp(uname_data.machine, "aarch64") == 0) {
			strncpy(architecture, "arm64", sizeof(architecture)); /* 64-bit ARM */
			goto done; /* Skip manual architecture selection */
		} else if (strcmp(uname_data.machine, "armv7l") == 0 ||
			   strcmp(uname_data.machine, "armv8l") == 0) {
			strncpy(architecture, "arm32", sizeof(architecture)); /* 32-bit ARM */
			goto done; /* Skip manual architecture selection */
		}

		/* Auto-detection failed - show detected architecture and prompt manual selection */
		printf("Unknown arch: %s\n", uname_data.machine);

/* Label for architecture selection retry */
back:
        /* Display architecture selection menu */
        printf("\033[1;33m== Select architecture for Termux ==\033[0m\n");
		printf("-> [A/a] arm32\n"); /* 32-bit ARM option */
		printf("-> [B/b] arm64\n"); /* 64-bit ARM option */

		char *selection = readline("==> "); /* Get user's architecture choice */

		/* Process architecture selection with case-insensitive string matching */
		if (strfind(selection, "A", true)) /* Check for 'A' or 'a' */
		{
			strncpy(architecture, "arm32", sizeof(architecture));
		} else if (strfind(selection, "B", true)) { /* Check for 'B' or 'b' */
			strncpy(architecture, "arm64", sizeof(architecture));
		} else {
			/* Invalid architecture selection */
			wg_free(selection);
			if (wgconfig.wg_sel_stat == 0) /* Make sure in interactive selection mode */
				return 0; /* Exit if not in selection mode */
			pr_error(stdout, "Invalid architecture selection");
			goto back; /* Retry architecture selection */
		}
		wg_free(selection);

/* Label for constructing download URL after selections are made */
done:
		/* Construct GitHub release download URL for Termux Pawn compiler */
		snprintf(url, sizeof(url),
			 "https://github.com/"
			 "mxp96/" /* Repository owner */
			 "compiler/" /* Repository name */
			 "releases/"
			 "download/"
			 "%s/" /* Version tag (e.g., 3.10.11) */
			 "pawnc-%s-%s.zip", /* Filename pattern: pawnc-version-architecture.zip */
			 library_termux_versions[version_index], /* Selected version */
			 library_termux_versions[version_index], /* Version in filename */
			 architecture); /* Selected architecture (arm32/arm64) */

		/* Construct local filename for downloaded archive */
		snprintf(filename, sizeof(filename), "pawncc-%s-%s.zip",
				    library_termux_versions[version_index], architecture);

		wgconfig.wg_ipawncc = 1; /* Set flag indicating PawnCC installation is in progress */
		wg_download_file(url, filename); /* Initiate the download process */

		return 0; /* Success */
}

/*
 * Handles standard Pawn compiler installation for Linux and Windows platforms
 * Provides interactive version selection from official compiler repositories
 * Supports both openmultiplayer (newer) and pawn-lang (older) repositories
 */
static int pawncc_handle_standard_installation(const char *platform)
{
		/* Available Pawn compiler versions for standard platforms */
		const char *versions[] = {
									"3.10.11", "3.10.10", "3.10.9", "3.10.8", "3.10.7",
									"3.10.6", "3.10.5", "3.10.4", "3.10.3", "3.10.2"
								 };
		const size_t version_count = sizeof(versions) / sizeof(versions[0]); /* 10 versions available */
		char version_selection; /* User's version choice character */
		char url[526]; /* Buffer for constructed download URL */
		char filename[128]; /* Buffer for local filename */
		const char *library_repo_base; /* Base GitHub repository URL */
		const char *archive_ext; /* File extension based on platform */
		int version_index; /* Index of selected version in array */

		/* Validate platform parameter - only Linux and Windows supported */
		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				return -1; /* Return error for unsupported platform */
		}

        /* Display version selection menu */
        printf("\033[1;33m== Select the PawnCC version to download ==\033[0m\n");
		/* List all available versions with letter options */
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s\n",
				(int)('A' + i), /* Uppercase option */
				(int)('a' + i), /* Lowercase option */
				versions[i]); /* Version number */
		}

		char *__version__; /* Buffer for user's version selection */

/* Label for version selection retry */
get_back:
		__version__ = readline("==> "); /* Get user input */
		version_selection = __version__[0]; /* Only consider first character */
		
		/* Convert selection character to array index (A-J or a-j) */
		if (version_selection >= 'A' &&
			version_selection <= 'J') { /* Uppercase A through J */
			version_index = version_selection - 'A'; /* A=0, B=1, ..., J=9 */
		} else if (version_selection >= 'a' &&
			version_selection <= 'j') { /* Lowercase a through j */
			version_index = version_selection - 'a'; /* a=0, b=1, ..., j=9 */
		} else {
			/* Invalid selection */
			wg_free(__version__);
			if (wgconfig.wg_sel_stat == 0) /* Check selection mode */
				return 0; /* Exit if not in selection mode */
			pr_error(stdout, "Invalid version selection");
			goto get_back; /* Retry version selection */
		}
		wg_free(__version__);

		/* Determine which GitHub repository to use based on version */
		if (strcmp(versions[version_index], "3.10.11") == 0)
				/* Version 3.10.11 is hosted in openmultiplayer repository */
				library_repo_base = "https://github.com/"
								"openmultiplayer/"
								"compiler";
		else
				/* Older versions are hosted in pawn-lang repository */
				library_repo_base = "https://github.com/"
								"pawn-lang/"
								"compiler";

		/* Determine archive extension based on platform */
		archive_ext = (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

		/* Construct GitHub release download URL */
		snprintf(url, sizeof(url),
			 "%s/releases/download/v%s/pawnc-%s-%s.%s",
			 library_repo_base, /* Base repository URL */
			 versions[version_index], /* Version tag (v3.10.11, etc.) */
			 versions[version_index], /* Version in filename */
			 platform, /* Target platform (linux/windows) */
			 archive_ext); /* Archive extension (tar.gz/zip) */

		/* Construct local filename for downloaded archive */
		snprintf(filename, sizeof(filename), "pawnc-%s-%s.%s",
				    versions[version_index], platform, archive_ext);

		wgconfig.wg_ipawncc = 1; /* Set PawnCC installation flag */
		wg_download_file(url, filename); /* Initiate download */

		return 0; /* Success */
}

/*
 * Main Pawn compiler installation entry point
 * Routes to appropriate installer based on platform (Termux, Linux, Windows)
 * Includes comprehensive debug information when debugging is enabled
 */
int wg_install_pawncc(const char *platform)
{
		/* Debug information section */
        __debug_function();
		
		/* Validate platform parameter is not NULL */
		if (!platform) {
				pr_error(stdout, "Platform parameter is NULL");
				if (wgconfig.wg_sel_stat == 0) /* Make sure in selection mode */
					return 0; /* Silent exit if not in interactive mode */
				return -1; /* Error exit */
		}
		
		/* Route to Termux-specific installer */
		if (strcmp(platform, "termux") == 0) {
			int ret = pawncc_handle_termux_installation(); /* Call Termux installer */
			
/* Label for installation retry loop (Termux) */
loop_ipcc:
			if (ret == -1 && wgconfig.wg_sel_stat != 0) /* Retry on error in interactive mode */
				goto loop_ipcc;
			else if (ret == 0) /* Success */
				return 0;
		} else {
			/* Route to standard platform installer */
			int ret = pawncc_handle_standard_installation(platform); /* Call standard installer */
			
/* Label for installation retry loop (Standard) */
loop_ipcc2:
			if (ret == -1 && wgconfig.wg_sel_stat != 0) /* Retry on error in interactive mode */
				goto loop_ipcc2;
			else if (ret == 0) /* Success */
				return 0;
		}

		return 0; /* Success */
}

/*
 * SA-MP and Open.MP server installation function
 * Provides interactive menu for selecting server type and version
 * Downloads server binaries from GitHub repositories
 * Supports both SA-MP (KrustyKoyle archive) and Open.MP (official releases)
 */
int wg_install_server(const char *platform)
{
		/* Debug information section */
        __debug_function();

		/* Structure to hold version information and download URLs */
		struct library_version_info {
				char key;                    /* Selection key (A, B, C, etc.) */
				const char *name;            /* Display name (e.g., "SA-MP 0.3.DL R1") */
				const char *linux_url;       /* Linux download URL */
				const char *linux_file;      /* Linux filename */
				const char *windows_url;    /* Windows download URL */
				const char *windows_file;   /* Windows filename */
		};

		/* Array of available server versions with metadata */
		struct library_version_info versions[] = {
				/* SA-MP 0.3.DL R1 - First row version */
				{
						'A', "SA-MP 0.3.DL R1", /* Key A, display name */
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DLsvr_R1.tar.gz",
						"samp03DLsvr_R1.tar.gz", /* Linux archive */
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DL_svr_R1_win32.zip",
						"samp03DL_svr_R1_win32.zip" /* Windows archive */
				},
				/* SA-MP 0.3.7 R3 - Popular stable version */
				{
						'B', "SA-MP 0.3.7 R3",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R3.tar.gz",
						"samp037svr_R3.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R3_win32.zip",
						"samp037_svr_R3_win32.zip"
				},
				/* SA-MP 0.3.7 R2-2-1 - Intermediate version */
				{
						'C', "SA-MP 0.3.7 R2-2-1",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-2-1.tar.gz",
						"samp037svr_R2-2-1.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-2-1_win32.zip"
				},
				/* SA-MP 0.3.7 R2-1-1 - Older stable version */
				{
						'D', "SA-MP 0.3.7 R2-1-1",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-1.tar.gz",
						"samp037svr_R2-1.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-1-1_win32.zip"
				},
				/* Open.MP v1.4.0.2779 - Latest Open.MP version at time of writing */
				{
						'E', "OPEN.MP v1.4.0.2779",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz", /* Linux Open.MP */
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
						"open.mp-win-x86.zip" /* Windows Open.MP */
				},
				/* Open.MP v1.3.1.2748 - Previous Open.MP version */
				{
						'F', "OPEN.MP v1.3.1.2748",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.2.0.2670 - Older Open.MP version */
				{
						'G', "OPEN.MP v1.2.0.2670",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				/* Open.MP v1.1.0.2612 - Early Open.MP version */
				{
						'H', "OPEN.MP v1.1.0.2612",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				}
		};

		const size_t version_count = sizeof(versions) / sizeof(versions[0]); /* 8 versions total */
		char selection; /* User's selection character */
		struct library_version_info *chosen = NULL; /* Pointer to selected version */
		size_t i; /* Loop counter */

		/* Validate platform parameter */
		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				if (wgconfig.wg_sel_stat == 0) /* Check selection mode */
					return 0; /* Silent exit */
				return -1; /* Error exit */
		}

        /* Display server version selection menu */
        printf("\033[1;33m== Select the SA-MP version to download ==\033[0m\n");
		/* List all available versions with their selection keys */
		for (i = 0; i < version_count; i++) {
				printf("-> [%c/%c] %s\n", versions[i].key, versions[i].key + 32,
				       versions[i].name); /* key+32 converts uppercase to lowercase */
		}

		char *__selection__; /* Buffer for user's selection input */

/* Label for selection retry */
get_back:
		__selection__ = readline("==> "); /* Get user input */
		selection = __selection__[0]; /* Only consider first character */
		
		/* Find the selected version in the array */
		for (i = 0; i < version_count; i++) {
			if (selection == versions[i].key || /* Uppercase match */
				selection == versions[i].key + 32) { /* Lowercase match (key + 32 = lowercase) */
				chosen = &versions[i]; /* Set pointer to selected version */
				break;
			}
		}
		
		/* Validate selection was found */
		if (!chosen) {
			wg_free(__selection__); /* Free invalid input */
			if (wgconfig.wg_sel_stat == 0) /* Check selection mode */
				return 0; /* Silent exit */
			pr_error(stdout, "Invalid selection");
			goto get_back; /* Retry selection */
		}
		wg_free(__selection__); /* Free valid input */

		/* Select appropriate URL and filename based on platform */
		const char *url = (strcmp(platform, "linux") == 0) ?
				chosen->linux_url : chosen->windows_url;
		const char *filename = (strcmp(platform, "linux") == 0) ?
				chosen->linux_file : chosen->windows_file;

		/* Initiate download process */
		wg_download_file(url, filename);

		return 0; /* Success */
}