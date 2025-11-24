#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "wd_extra.h"
#include "wd_util.h"

#ifndef WD_WINDOWS
#include <sys/utsname.h>
#endif

#include "wd_unit.h"
#include "wd_archive.h"
#include "wd_curl.h"
#include "wd_hardware.h"
#include "wd_package.h"

struct version_info {
		char key;
		const char *name;
		const char *linux_url;
		const char *linux_file;
		const char *windows_url;
		const char *windows_file;
};

static int pawncc_handle_termux_installation(void)
{
		const char *termux_versions[] = {
							"3.10.11",
							"3.10.10"
						};
		size_t version_count;
		version_count = sizeof(termux_versions) / sizeof(termux_versions[0]);
		char version_selection;
		char architecture[16];
		char url[WD_PATH_MAX * 3];
		char filename[128];
		int version_index;

		if (!is_termux_environment())
				pr_info(stdout, "Currently not in Termux!");

		char *__version__;
ret_pawncc:
		printf("Select the PawnCC version to download:\n");
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s (mxp96)\n",
				(int)('A' + i),
				(int)('a' + i),
				termux_versions[i]);
		}

		__version__ = readline("==> ");
		version_selection = __version__[0];
		if (version_selection >= 'A' &&
			version_selection < 'A' + (char)version_count) {
			version_index = version_selection - 'A';
		} else if (version_selection >= 'a' &&
			version_selection < 'a' + (char)version_count) {
			version_index = version_selection - 'a';
		} else {
			pr_error(stdout, "Invalid version selection '%c'. "
 					 "Input must be A..%c or a..%c\n",
				version_selection,
				'A' + (int)version_count - 1,
				'a' + (int)version_count - 1);
			wd_free(__version__);
			goto ret_pawncc;
	    	}
		wd_free(__version__);

		struct utsname uname_data;

		if (uname(&uname_data) != 0) {
				pr_error(stdout, "Failed to get system information");
				return -WD_RETN;
		}

		if (strcmp(uname_data.machine, "aarch64") == 0) {
			wd_strncpy(architecture, "arm64", sizeof(architecture));
			goto done;
		} else if (strcmp(uname_data.machine, "armv7l") == 0 ||
			   strcmp(uname_data.machine, "armv8l") == 0) {
			wd_strncpy(architecture, "arm32", sizeof(architecture));
			goto done;
		}

		printf("Unknown arch: %s\n", uname_data.machine);

back:
		printf("Select architecture for Termux:\n");
		printf("-> [A/a] arm32\n");
		printf("-> [B/b] arm64\n");

		char *selection = readline("==> ");

		if (strfind(selection, "A"))
		{
			wd_strncpy(architecture, "arm32", sizeof(architecture));
			wd_free(selection);
		} else if (strfind(selection, "B")) {
			wd_strncpy(architecture, "arm64", sizeof(architecture));
			wd_free(selection);
		} else {
			wd_free(selection);
			if (wcfg.wd_sel_stat == 0)
				return WD_RETZ;
			pr_error(stdout, "Invalid architecture selection");
			goto back;
		}
		wd_free(selection);

done:
		wd_snprintf(url, sizeof(url),
			 "https://github.com/"
			 "mxp96/"
			 "compiler/"
			 "releases/"
			 "download/"
			 "%s/"
			 "pawnc-%s-%s.zip",
			 termux_versions[version_index],
			 termux_versions[version_index],
			 architecture);

		wd_snprintf(filename, sizeof(filename), "pawncc-%s-%s.zip",
				    termux_versions[version_index], architecture);

		wcfg.wd_ipawncc = 1;
		wd_download_file(url, filename);

		return WD_RETZ;
}

static int pawncc_handle_standard_installation(const char *platform)
{
		const char *versions[] = {
									"3.10.11", "3.10.10", "3.10.9", "3.10.8", "3.10.7",
									"3.10.6", "3.10.5", "3.10.4", "3.10.3", "3.10.2"
								 };
		const size_t version_count = sizeof(versions) / sizeof(versions[0]);
		char version_selection;
		char url[526];
		char filename[128];
		const char *pkg_repo_base;
		const char *archive_ext;
		int version_index;

		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				return -WD_RETN;
		}

		printf("Select the PawnCC version to download:\n");
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s\n",
				(int)('A' + i),
				(int)('a' + i),
				versions[i]);
		}

		char *__version__;
get_back:
		__version__ = readline("==> ");
		version_selection = __version__[0];
		if (version_selection >= 'A' &&
			version_selection <= 'J') {
			version_index = version_selection - 'A';
		} else if (version_selection >= 'a' &&
			version_selection <= 'j') {
			version_index = version_selection - 'a';
		} else {
			wd_free(__version__);
			if (wcfg.wd_sel_stat == 0)
				return WD_RETZ;
			pr_error(stdout, "Invalid version selection");
			goto get_back;
		}
		wd_free(__version__);

		if (strcmp(versions[version_index], "3.10.11") == 0)
				pkg_repo_base = "https://github.com/"
								"openmultiplayer/"
								"compiler";
		else
				pkg_repo_base = "https://github.com/"
								"pawn-lang/"
								"compiler";

		archive_ext = (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

		wd_snprintf(url, sizeof(url),
			 "%s/releases/download/v%s/pawnc-%s-%s.%s",
			 pkg_repo_base, versions[version_index], versions[version_index],
			 platform, archive_ext);

		wd_snprintf(filename, sizeof(filename), "pawnc-%s-%s.%s",
				    versions[version_index], platform, archive_ext);

		wcfg.wd_ipawncc = 1;
		wd_download_file(url, filename);

		return WD_RETZ;
}

int wd_install_pawncc(const char *platform)
{
        	/* Debugging Pawncc Installation Function */
#if defined (_DBG_PRINT)
		pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
	    printf("[function: %s | "
               "pretty function: %s | "
               "line: %d | "
               "file: %s | "
               "date: %s | "
               "time: %s | "
               "timestamp: %s | "
               "C standard: %ld | "
               "C version: %s | "
               "compiler version: %d | "
               "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
		if (!platform) {
				pr_error(stdout, "Platform parameter is NULL");
				if (wcfg.wd_sel_stat == 0)
					return WD_RETZ;
				return -WD_RETN;
		}
		if (strcmp(platform, "termux") == 0) {
			int ret = pawncc_handle_termux_installation();
loop_ipcc:
			if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
				goto loop_ipcc;
			else if (ret == WD_RETZ)
				return WD_RETZ;
		} else {
			int ret = pawncc_handle_standard_installation(platform);
loop_ipcc2:
			if (ret == -WD_RETN && wcfg.wd_sel_stat != 0)
				goto loop_ipcc2;
			else if (ret == WD_RETZ)
				return WD_RETZ;
		}

		return WD_RETZ;
}

int wd_install_server(const char *platform)
{
        /* Debugging Server Installation Function */
#if defined (_DBG_PRINT)
		pr_color(stdout, FCOLOUR_YELLOW, "-DEBUGGING ");
	    printf("[function: %s | "
               "pretty function: %s | "
               "line: %d | "
               "file: %s | "
               "date: %s | "
               "time: %s | "
               "timestamp: %s | "
               "C standard: %ld | "
               "C version: %s | "
               "compiler version: %d | "
               "architecture: %s]:\n",
                __func__, __PRETTY_FUNCTION__,
                __LINE__, __FILE__,
                __DATE__, __TIME__,
                __TIMESTAMP__,
                __STDC_VERSION__,
                __VERSION__,
                __GNUC__,
#ifdef __x86_64__
                "x86_64");
#elif defined(__i386__)
                "i386");
#elif defined(__arm__)
                "ARM");
#elif defined(__aarch64__)
                "ARM64");
#else
                "Unknown");
#endif
#endif
		struct version_info versions[] = {
				{
						'A', "SA-MP 0.3.DL R1",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DLsvr_R1.tar.gz",
						"samp03DLsvr_R1.tar.gz",
						"https://github.com/"
						"KrustyKoyle/"
						"files.sa-mp.com-Archive/raw/refs/heads/master/samp03DL_svr_R1_win32.zip",
						"samp03DL_svr_R1_win32.zip"
				},
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
				{
						'E', "OPEN.MP v1.4.0.2779",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/"
						"openmultiplayer/"
						"open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
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

		const size_t version_count = sizeof(versions) / sizeof(versions[0]);
		char selection;
		struct version_info *chosen = NULL;
		size_t i;

		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				if (wcfg.wd_sel_stat == 0)
					return WD_RETZ;
				return -WD_RETN;
		}

		printf("Select the SA-MP version to download:\n");
		for (i = 0; i < version_count; i++) {
				printf("-> [%c/%c] %s\n", versions[i].key, versions[i].key + 32,
				       versions[i].name);
		}

		char *__selection__;
get_back:
		__selection__ = readline("==> ");
		selection = __selection__[0];
		for (i = 0; i < version_count; i++) {
			if (selection == versions[i].key ||
				selection == versions[i].key + 32) {
				chosen = &versions[i];
				break;
			}
		}
		if (!chosen) {
			wd_free(__selection__);
			if (wcfg.wd_sel_stat == 0)
				return WD_RETZ;
			pr_error(stdout, "Invalid selection");
			goto get_back;
		}
		wd_free(__selection__);

		const char *url = (strcmp(platform, "linux") == 0) ?
				chosen->linux_url : chosen->windows_url;
		const char *filename = (strcmp(platform, "linux") == 0) ?
				chosen->linux_file : chosen->windows_file;

		wd_download_file(url, filename);

		return WD_RETZ;
}
