#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stddef.h>
#include "extra.h"
#include "utils.h"

#ifndef WG_WINDOWS
	#include <sys/utsname.h>
#endif

#include "units.h"
#include "archive.h"
#include "curl.h"
#include "lowlevel.h"
#include "package.h"

static int pawncc_handle_termux_installation(void)
{
		int version_index;
		char version_selection, architecture[16],
			 url[WG_PATH_MAX * 3], filename[128];
		const char *pkg_termux_versions[] = { "3.10.11", "3.10.10" };
		size_t size_pkg_termux_versions = sizeof(pkg_termux_versions);
		size_t size_pkg_termux_versions_zero = sizeof(pkg_termux_versions[0]);
		size_t version_count = size_pkg_termux_versions / size_pkg_termux_versions_zero;

		if (!is_termux_env())
				pr_info(stdout, "Currently not in Termux!");

		char *__version__;
ret_pawncc:
        printf("\033[1;33m== Select the PawnCC version to download ==\033[0m\n");
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s (mxp96)\n",
				(int)('A' + i),
				(int)('a' + i),
				pkg_termux_versions[i]);
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
			wg_free(__version__);
			goto ret_pawncc;
	    	}
		wg_free(__version__);

		struct utsname uname_data;

		if (uname(&uname_data) != 0) {
				pr_error(stdout, "Failed to get system information");
				return -1;
		}

		if (strcmp(uname_data.machine, "aarch64") == 0) {
			wg_strncpy(architecture, "arm64", sizeof(architecture));
			goto done;
		} else if (strcmp(uname_data.machine, "armv7l") == 0 ||
			   strcmp(uname_data.machine, "armv8l") == 0) {
			wg_strncpy(architecture, "arm32", sizeof(architecture));
			goto done;
		}

		printf("Unknown arch: %s\n", uname_data.machine);

back:
        printf("\033[1;33m== Select architecture for Termux ==\033[0m\n");
		printf("-> [A/a] arm32\n");
		printf("-> [B/b] arm64\n");

		char *selection = readline("==> ");

		if (strfind(selection, "A", false))
		{
			wg_strncpy(architecture, "arm32", sizeof(architecture));
			wg_free(selection);
		} else if (strfind(selection, "B", false)) {
			wg_strncpy(architecture, "arm64", sizeof(architecture));
			wg_free(selection);
		} else {
			wg_free(selection);
			if (wgconfig.wg_sel_stat == 0)
				return 0;
			pr_error(stdout, "Invalid architecture selection");
			goto back;
		}
		wg_free(selection);

done:
		wg_snprintf(url, sizeof(url),
			 "https://github.com/"
			 "mxp96/"
			 "compiler/"
			 "releases/"
			 "download/"
			 "%s/"
			 "pawnc-%s-%s.zip",
			 pkg_termux_versions[version_index],
			 pkg_termux_versions[version_index],
			 architecture);

		wg_snprintf(filename, sizeof(filename), "pawncc-%s-%s.zip",
				    pkg_termux_versions[version_index], architecture);

		wgconfig.wg_ipawncc = 1;
		wg_download_file(url, filename);

		return 0;
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
				return -1;
		}

        printf("\033[1;33m== Select the PawnCC version to download ==\033[0m\n");
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
			wg_free(__version__);
			if (wgconfig.wg_sel_stat == 0)
				return 0;
			pr_error(stdout, "Invalid version selection");
			goto get_back;
		}
		wg_free(__version__);

		if (strcmp(versions[version_index], "3.10.11") == 0)
				pkg_repo_base = "https://github.com/"
								"openmultiplayer/"
								"compiler";
		else
				pkg_repo_base = "https://github.com/"
								"pawn-lang/"
								"compiler";

		archive_ext = (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

		wg_snprintf(url, sizeof(url),
			 "%s/releases/download/v%s/pawnc-%s-%s.%s",
			 pkg_repo_base, versions[version_index], versions[version_index],
			 platform, archive_ext);

		wg_snprintf(filename, sizeof(filename), "pawnc-%s-%s.%s",
				    versions[version_index], platform, archive_ext);

		wgconfig.wg_ipawncc = 1;
		wg_download_file(url, filename);

		return 0;
}

int wg_install_pawncc(const char *platform)
{
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
				if (wgconfig.wg_sel_stat == 0)
					return 0;
				return -1;
		}
		if (strcmp(platform, "termux") == 0) {
			int ret = pawncc_handle_termux_installation();
loop_ipcc:
			if (ret == -1 && wgconfig.wg_sel_stat != 0)
				goto loop_ipcc;
			else if (ret == 0)
				return 0;
		} else {
			int ret = pawncc_handle_standard_installation(platform);
loop_ipcc2:
			if (ret == -1 && wgconfig.wg_sel_stat != 0)
				goto loop_ipcc2;
			else if (ret == 0)
				return 0;
		}

		return 0;
}

int wg_install_server(const char *platform)
{
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
		struct pkg_version_info versions[] = {
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
		struct pkg_version_info *chosen = NULL;
		size_t i;

		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				if (wgconfig.wg_sel_stat == 0)
					return 0;
				return -1;
		}

        printf("\033[1;33m== Select the SA-MP version to download ==\033[0m\n");
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
			wg_free(__selection__);
			if (wgconfig.wg_sel_stat == 0)
				return 0;
			pr_error(stdout, "Invalid selection");
			goto get_back;
		}
		wg_free(__selection__);

		const char *url = (strcmp(platform, "linux") == 0) ?
				chosen->linux_url : chosen->windows_url;
		const char *filename = (strcmp(platform, "linux") == 0) ?
				chosen->linux_file : chosen->windows_file;

		wg_download_file(url, filename);

		return 0;
}
