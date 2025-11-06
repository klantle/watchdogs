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

#ifndef _WIN32
#include <sys/utsname.h>
#endif

#include "wd_unit.h"
#include "wd_util.h"
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

static int get_termux_architecture(char *out_arch, size_t buf_size)
{
		struct utsname uname_data;

		if (uname(&uname_data) != 0) {
				pr_error(stdout, "Failed to get system information");
				return -__RETN;
		}

		if (strcmp(uname_data.machine, "aarch64") == 0) {
				strncpy(out_arch, "arm64", buf_size);
				return __RETZ;
		} else if (strcmp(uname_data.machine, "armv7l") == 0 ||
				   strcmp(uname_data.machine, "armv8l") == 0) {
				strncpy(out_arch, "arm32", buf_size);
				return __RETZ;
		}

		printf("Unknown or unsupported architecture: %s\n", uname_data.machine);

		printf("Select architecture for Termux:\n");
		printf("-> [A/a] arm32\n");
		printf("-> [B/b] arm64\n");

		char *selection = readline("==> ");

		if (strfind(selection, "A"))
		{
			strncpy(out_arch, "arm32", buf_size);
			wd_free(selection);
			return __RETZ;
		} else if (strfind(selection, "B")) {
			strncpy(out_arch, "arm64", buf_size);
			wd_free(selection);
			return __RETZ;
		} else {
			pr_error(stdout, "Invalid architecture selection");
			if (wcfg.wd_sel_stat == 0)
				return __RETZ;
			wd_free(selection);
			return -__RETN;
		}
}

static int handle_termux_installation(void)
{
		const char *termux_versions[] = {
																				"3.10.11",
																				"3.10.10"
																		};
		const size_t version_count = sizeof(termux_versions) / sizeof(termux_versions[0]);
		char version_selection;
		char architecture[16];
		char url[526];
		char filename[128];
		int version_index;

		if (!is_termux_environment()) {
				pr_warning(stdout, "Currently not in Termux!");
		}

		printf("Select the PawnCC version to download:\n");
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s (mxp96)\n",
				(int)('A' + i),
				(int)('a' + i),
				termux_versions[i]);
		}

		char *__version__ = readline("==> ");
		version_selection = __version__[0];
		if (version_selection >= 'A' && version_selection < 'A' + (char)version_count) {
			version_index = version_selection - 'A';
		} else if (version_selection >= 'a' && version_selection < 'a' + (char)version_count) {
			version_index = version_selection - 'a';
		} else {
			printf("error: Invalid version selection '%c'. Input must be A..%c or a..%c\n",
				version_selection,
				'A' + (int)version_count - 1,
				'a' + (int)version_count - 1);
			wd_free(__version__);
			return -__RETN;;
    	}

		int ret = get_termux_architecture(architecture, sizeof(architecture) != 0);
		if (ret == __RETZ)
			return __RETZ;

		snprintf(url, sizeof(url),
				 "https://github.com/mxp96/compiler/releases/download/%s/pawnc-%s-%s.zip",
				 termux_versions[version_index], termux_versions[version_index], architecture);

		snprintf(filename, sizeof(filename), "pawncc-%s-%s.zip",
				 termux_versions[version_index], architecture);

		wcfg.wd_ipackage = 1;
		wd_download_file(url, filename);

		return __RETZ;
}

static int handle_standard_installation(const char *platform)
{
		const char *versions[] = {
									"3.10.11", "3.10.10", "3.10.9", "3.10.8", "3.10.7",
									"3.10.6", "3.10.5", "3.10.4", "3.10.3", "3.10.2"
								 };
		const size_t version_count = sizeof(versions) / sizeof(versions[0]);
		char version_selection;
		char url[526];
		char filename[128];
		const char *repo_base;
		const char *archive_ext;
		int version_index;

		if (strcmp(platform, "linux") != 0 && strcmp(platform, "windows") != 0) {
				pr_error(stdout, "Unsupported platform: %s", platform);
				return -__RETN;
		}

		printf("Select the PawnCC version to download:\n");
		for (size_t i = 0; i < version_count; i++) {
			printf("-> [%c/%c] PawnCC %s\n",
				(int)('A' + i),
				(int)('a' + i),
				versions[i]);
		}

		char *__version__ = readline("==> ");
		version_selection = __version__[0];
		if (version_selection >= 'A' && version_selection <= 'J') {
			version_index = version_selection - 'A';
		} else if (version_selection >= 'a' && version_selection <= 'j') {
			version_index = version_selection - 'a';
		} else {
			if (wcfg.wd_sel_stat == 0)
				return __RETZ;
			pr_error(stdout, "Invalid version selection");
			wd_free(__version__);
			return -__RETN;
		}

		if (strcmp(versions[version_index], "3.10.11") == 0) {
				repo_base = "https://github.com/openmultiplayer/compiler";
		} else {
				repo_base = "https://github.com/pawn-lang/compiler";
		}

		archive_ext = (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

		snprintf(url, sizeof(url),
				 "%s/releases/download/v%s/pawnc-%s-%s.%s",
				 repo_base, versions[version_index], versions[version_index],
				 platform, archive_ext);

		snprintf(filename, sizeof(filename), "pawnc-%s-%s.%s",
				 versions[version_index], platform, archive_ext);

		wcfg.wd_ipackage = 1;
		wd_download_file(url, filename);

		return __RETZ;
}

int wd_install_pawncc(const char *platform)
{
		if (!platform) {
				pr_error(stdout, "Platform parameter is NULL");
				if (wcfg.wd_sel_stat == 0)
					return __RETZ;
				return -__RETN;
		}
		if (strcmp(platform, "termux") == 0) {
			int ret = handle_termux_installation();
loop_ipcc:
			if (ret == -__RETN && wcfg.wd_sel_stat != 0)
				goto loop_ipcc;
			else if (ret == __RETZ)
				return __RETZ;
		} else {
			int ret = handle_standard_installation(platform);
loop_ipcc2:
			if (ret == -__RETN && wcfg.wd_sel_stat != 0)
				goto loop_ipcc2;
			else if (ret == __RETZ)
				return __RETZ;
		}

		return __RETZ;
}

int wd_install_server(const char *platform)
{
		struct version_info versions[] = {
				{
						'A', "SA-MP 0.3.DL R1",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp03DLsvr_R1.tar.gz",
						"samp03DLsvr_R1.tar.gz",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp03DL_svr_R1_win32.zip",
						"samp03DL_svr_R1_win32.zip"
				},
				{
						'B', "SA-MP 0.3.7 R3",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R3.tar.gz",
						"samp037svr_R3.tar.gz",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R3_win32.zip",
						"samp037_svr_R3_win32.zip"
				},
				{
						'C', "SA-MP 0.3.7 R2-2-1",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-2-1.tar.gz",
						"samp037svr_R2-2-1.tar.gz",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-2-1_win32.zip"
				},
				{
						'D', "SA-MP 0.3.7 R2-1-1",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-1.tar.gz",
						"samp037svr_R2-1.tar.gz",
						"https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
						"samp037_svr_R2-1-1_win32.zip"
				},
				{
						'E', "OPEN.MP v1.4.0.2779",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				{
						'F', "OPEN.MP v1.3.1.2748",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				{
						'G', "OPEN.MP v1.2.0.2670",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
						"open.mp-win-x86.zip"
				},
				{
						'H', "OPEN.MP v1.1.0.2612",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86.tar.gz",
						"open.mp-linux-x86.tar.gz",
						"https://github.com/openmultiplayer/open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
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
					return __RETZ;
				return -__RETN;
		}

		printf("Select the SA-MP version to download:\n");
		for (i = 0; i < version_count; i++) {
				printf("-> [%c/%c] %s\n", versions[i].key, versions[i].key + 32,
				       versions[i].name);
		}

		char *__selection__ = readline("==> ");

		selection = __selection__[0];
		for (i = 0; i < version_count; i++) {
				if (selection == versions[i].key || selection == versions[i].key + 32) {
						chosen = &versions[i];
						break;
				}
		}

		if (!chosen) {
				pr_error(stdout, "Invalid selection");
				if (wcfg.wd_sel_stat == 0)
					return __RETZ;
				wd_free(__selection__);
				return -__RETN;
		}

		const char *url = (strcmp(platform, "linux") == 0) ?
				chosen->linux_url : chosen->windows_url;
		const char *filename = (strcmp(platform, "linux") == 0) ?
				chosen->linux_file : chosen->windows_file;

		wd_download_file(url, filename);

		return __RETZ;
}
