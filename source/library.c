/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#include  "utils.h"
#include  "units.h"
#include  "archive.h"
#include  "curl.h"
#include  "debug.h"
#include  "library.h"

bool
	installing_pawncc = false;

static char
library_options_list(const char *title, const char **items,
    const char *keys, int counts)
{
	if (title[0] != '\0')
		printf("\033[1;33m== %s ==\033[0m\n", title);

	int	 i;
	for (i = 0; i < counts; i++) {
		printf("  %c) %s\n", keys[i], items[i]);
	}

	while (true) {
		char	*input = NULL;
		printf(DOG_COL_CYAN ">" DOG_COL_DEFAULT);
		input = readline(" ");
		if (!input)
			continue;

		char	 choice = '\0';
		if (strlen(input) == 1) {
			choice = input[0];
			int	 k;
			for (k = 0; k < counts; k++)
				if (choice == keys[k] ||
				    choice == (keys[k] + 32))
				{
					dog_free(input);
					return (choice);
				}
		}

		printf("Invalid selection. Please try again.\n");
		dog_free(input);
	}
}

static int
pawncc_handle_termux_installation(void)
{
	const char	*items[] = {
		"Pawncc 3.10.11  - new",
		"Pawncc 3.10.10  - new",
		"Pawncc 3.10.9   - new",
		"Pawncc 3.10.8   - stable",
		"Pawncc 3.10.7   - stable"
	};
	const char	 keys[] = {'A', 'B', 'C', 'D', 'E'};

	char	 sel = library_options_list("Select PawnCC Version",
	    items, keys, 5);
	if (!sel)
		return (0);

	installing_pawncc = true;

	if (sel == 'A' || sel == 'a') {
		if (path_exists("pawncc-termux-311.zip"))
			remove("pawncc-termux-311.zip");
		if (path_exists("pawncc-termux-311"))
			remove("pawncc-termux-311");
		dog_download_file(
		    "https://github.com/gskeleton/compiler/releases/download/3.10.11/pawncc-termux.zip",
		    "pawncc-termux-311.zip");
	} else if (sel == 'B' || sel == 'b') {
		if (path_exists("pawncc-termux-310.zip"))
			remove("pawncc-termux-310.zip");
		if (path_exists("pawncc-termux-310"))
			remove("pawncc-termux-310");
		dog_download_file(
		    "https://github.com/gskeleton/compiler/releases/download/3.10.10/pawncc-termux.zip",
		    "pawncc-termux-310.zip");
	} else if (sel == 'C' || sel == 'c') {
		if (path_exists("pawncc-termux-39.zip"))
			remove("pawncc-termux-39.zip");
		if (path_exists("pawncc-termux-39"))
			remove("pawncc-termux-39");
		dog_download_file(
		    "https://github.com/gskeleton/compiler/releases/download/3.10.9/pawncc-termux.zip",
		    "pawncc-termux-39.zip");
	} else if (sel == 'D' || sel == 'd') {
		if (path_exists("pawncc-termux-38.zip"))
			remove("pawncc-termux-38.zip");
		if (path_exists("pawncc-termux-38"))
			remove("pawncc-termux-38");
		dog_download_file(
		    "https://github.com/gskeleton/compiler/releases/download/3.10.8/pawncc-termux.zip",
		    "pawncc-termux-38.zip");
	} else if (sel == 'E' || sel == 'e') {
		if (path_exists("pawncc-termux-37.zip"))
			remove("pawncc-termux-37.zip");
		if (path_exists("pawncc-termux-37"))
			remove("pawncc-termux-37");
		dog_download_file(
		    "https://github.com/gskeleton/compiler/releases/download/3.10.7/pawncc-termux.zip",
		    "pawncc-termux-37.zip");
	}

	return (0);
}

static int
pawncc_handle_standard_installation(const char *platform)
{
	const char	*versions[] = {
		"PawnCC 3.10.11  - new",
		"PawnCC 3.10.10  - new",
		"PawnCC 3.10.9   - new",
		"PawnCC 3.10.8   - stable",
		"PawnCC 3.10.7   - stable",
		"PawnCC 3.10.6   - older",
		"PawnCC 3.10.5   - older",
		"PawnCC 3.10.4   - older",
		"PawnCC 3.10.3   - older",
		"PawnCC 3.10.2   - older"
	};
	const char	 keys[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G',
	    'H', 'I', 'J'};
	const char	*vernums[] = {
		"3.10.11", "3.10.10", "3.10.9", "3.10.8", "3.10.7",
		"3.10.6", "3.10.5", "3.10.4", "3.10.3", "3.10.2"
	};

	if (strcmp(platform, "linux") != 0 &&
	    strcmp(platform, "windows") != 0) {
		pr_error(stdout, "Unsupported platform: %s", platform);
		return (-1);
	}

	char	 sel = library_options_list("Select PawnCC Version",
	    versions, keys, 10);
	if (!sel)
		return (0);

	int	 idx = -1;
	if (sel >= 'A' && sel <= 'J')
		idx = sel - 'A';
	else if (sel >= 'a' && sel <= 'j')
		idx = sel - 'a';
	if (idx < 0 || idx >= 10)
		return (0);

	const char	*library_repo_base;
	if (strcmp(vernums[idx], "3.10.11") == 0)
		library_repo_base =
		    "https://github.com/openmultiplayer/compiler";
	else
		library_repo_base = "https://github.com/pawn-lang/compiler";

	const char	*archive_ext =
	    (strcmp(platform, "linux") == 0) ? "tar.gz" : "zip";

	char	 url[512], filename[128];
	snprintf(url, sizeof(url),
	    "%s/releases/download/v%s/pawnc-%s-%s.%s",
	    library_repo_base, vernums[idx], vernums[idx], platform,
	    archive_ext);

	snprintf(filename, sizeof(filename),
	    "pawnc-%s-%s.%s", vernums[idx], platform, archive_ext);

	installing_pawncc = true;

	dog_download_file(url, filename);

	return (0);
}

int
dog_install_pawncc(const char *platform)
{
	minimal_debugging();

	bool	 stat_false
	    = !unit_selection_stat;

	if (!platform) {
		pr_error(stdout, "Platform parameter is NULL");
		if (stat_false)
			return (0);
		return (-1);
	}

	if (strcmp(platform, "termux") == 0) {
		int	 ret = pawncc_handle_termux_installation();

loop_ipcc:
		if (stat_false)
			goto loop_ipcc;
		else if (ret == 0)
			return (0);
	} else {
		int	 ret = pawncc_handle_standard_installation(platform);

loop_ipcc2:
		if (stat_false)
			goto loop_ipcc2;
		else if (ret == 0)
			return (0);
	}

	return (0);
}

int
dog_install_server(const char *platform)
{
	minimal_debugging();

	if (strcmp(platform, "linux") != 0 &&
	    strcmp(platform, "windows") != 0 &&
	    strcmp(platform, "termux") != 0) {
		pr_error(stdout, "Unsupported platform: %s", platform);
		return (-1);
	}

	if (strcmp(platform, "termux") == 0) {
		const char	*items[] = {
			"OMPTMUX (open.mp termux) v1.5.8.3079",
		};
		const char	 keys[] = {'A'};

		char	 sel = library_options_list("Select Server Version",
		    items, keys, 1);
		if (!sel)
			return (0);

		if (sel == 'A' || sel == 'a') {
			if (path_exists("omptmux.zip"))
				remove("omptmux.zip");
			dog_download_file(
			    "https://github.com/novusr/omptmux/"
			    "releases/download/v1.5.8.3079/open.mp-termux-aarch64.zip",
			    "omptmux.zip");
		}
		goto done;
	}

	const char	*items[] = {
		"SA-MP 0.3.DL R1",
		"SA-MP 0.3.7 R3",
		"SA-MP 0.3.7 R2-2-1",
		"SA-MP 0.3.7 R2-1-1",
		"OPEN.MP v1.5.8.3079 (Static SSL)",
		"OPEN.MP v1.5.8.3079 (Dynamic SSL)",
		"OPEN.MP v1.4.0.2779 (Static SSL)",
		"OPEN.MP v1.4.0.2779 (Dynamic SSL)",
		"OPEN.MP v1.3.1.2748 (Static SSL)",
		"OPEN.MP v1.3.1.2748 (Dynamic SSL)",
		"OPEN.MP v1.2.0.2670 (Static SSL)",
		"OPEN.MP v1.2.0.2670 (Dynamic SSL)",
		"OPEN.MP v1.1.0.2612 (Static SSL)",
		"OPEN.MP v1.1.0.2612 (Dynamic SSL)"
	};

	const char	 keys[] = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
		'M', 'N'
	};

	char	 sel = library_options_list("Select SA-MP / open.mp Server",
	    items, keys, 14);
	if (!sel)
		return (0);

	int	 idx = -1;
	if (sel >= 'A' && sel <= 'N')
		idx = sel - 'A';
	else if (sel >= 'a' && sel <= 'n')
		idx = sel - 'a';
	if (idx < 0 || idx >= 14)
		return (0);

	struct library_version_info versions[] = {
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
			'E', "OPEN.MP v1.5.8.3079 (Static SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.5.8.3079/open.mp-linux-x86.tar.gz",
			"open.mp-linux-x86.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.5.8.3079/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'F', "OPEN.MP v1.5.8.3079 (Dynamic SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.5.8.3079/open.mp-linux-x86-dynssl.tar.gz",
			"open.mp-linux-x86-dynssl.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.5.8.3079/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'G', "OPEN.MP v1.4.0.2779 (Static SSL)",
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
			'H', "OPEN.MP v1.4.0.2779 (Dynamic SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86-dynssl.tar.gz",
			"open.mp-linux-x86-dynssl.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'I', "OPEN.MP v1.3.1.2748 (Static SSL)",
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
			'J', "OPEN.MP v1.3.1.2748 (Dynamic SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86-dynssl.tar.gz",
			"open.mp-linux-x86-dynssl.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'K', "OPEN.MP v1.2.0.2670 (Static SSL)",
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
			'L', "OPEN.MP v1.2.0.2670 (Dynamic SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86-dynssl.tar.gz",
			"open.mp-linux-x86-dynssl.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'M', "OPEN.MP v1.1.0.2612 (Static SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86.tar.gz",
			"open.mp-linux-x86.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		},
		{
			'N', "OPEN.MP v1.1.0.2612 (Dynamic SSL)",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86-dynssl.tar.gz",
			"open.mp-linux-x86-dynssl.tar.gz",
			"https://github.com/"
			"openmultiplayer/"
			"open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
			"open.mp-win-x86.zip"
		}
	};

	struct library_version_info *chosen = &versions[idx];

	const char	*url = (strcmp(platform, "linux") == 0) ?
	    chosen->linux_url : chosen->windows_url;
	const char	*filename = (strcmp(platform, "linux") == 0) ?
	    chosen->linux_file : chosen->windows_file;

	dog_download_file(url, filename);
done:
	return (0);
}
