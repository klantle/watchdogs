#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <stddef.h>

#include "chain.h"
#include "utils.h"
#include "archive.h"
#include "curl.h"
#include "package.h"

typedef struct {
        char key;
        const char *name;
        const char *linux_url;
        const char *linux_file;
        const char *windows_url;
        const char *windows_file;
} VersionInfo;

void
watch_pawncc(const char *platform) {
        char selection,
             version_selection,
             url_sel[300],
             fname_sel[256];

        if (strcmp(platform, "termux") == 0) {
            struct stat st;
            int is_termux = 0;
            if (!stat("/data/data/com.termux/files/usr/local/lib/", &st) ||
                !stat("/data/data/com.termux/files/usr/lib/", &st) &&
                S_ISDIR(st.st_mode)) {
                    is_termux=1;
                }
            if (is_termux == 0)
            {
                char verify_first;
                printf(":: You are no longer in Termux!. do you continue? [y/N] ");
                if (scanf(" %c", &verify_first) != 1) return;
                if (verify_first == 'N' || verify_first == 'n') {
                    __init(0);
                    return;
                }
            }

            const char *termux_versions[] = { "3.10.11", "3.10.10" };
            int version_count = 2;
            printf("Select the PawnCC version to download (Termux build by mxp96):\n");
            for (int i = 0; i < version_count; i++)
                printf("[%c/%c] PawnCC %s\n", 'A'+i, 'a'+i, termux_versions[i]);

            printf(">> ");
            if (scanf(" %c", &version_selection) != 1) return;

            int pcc_sel_index = (version_selection >= 'A' && version_selection < 'A' + version_count)
                ? version_selection - 'A'
                : (version_selection >= 'a' && version_selection < 'a' + version_count)
                ? version_selection - 'a'
                : -1;

            if (pcc_sel_index < 0 || pcc_sel_index >= version_count) {
                printf("Invalid selection.\n");
                return;
            }

            struct utsname uname_data;
            uname(&uname_data);
            const char *arch = uname_data.machine;

            const char *detected_arch;
            if (strcmp(arch, "aarch64") == 0) {
                detected_arch = "arm64";
            } else if (strcmp(arch, "armv7l") == 0 || strcmp(arch, "armv8l") == 0) {
                detected_arch = "arm32";
            } else {
                printf("Unknown or unsupported architecture: %s\n", arch);
                char confirm_arch;
                printf("Do you want to select architecture manually? [y/N] ");
                if (scanf(" %c", &confirm_arch) != 1 || (confirm_arch != 'y' && confirm_arch != 'Y')) {
                    return;
                }
                char arch_selection;
                printf("Select architecture for Termux:\n[A/a] arm32\n[B/b] arm64\n>> ");
                if (scanf(" %c", &arch_selection) != 1) return;

                if (arch_selection == 'A' || arch_selection == 'a') detected_arch = "arm32";
                else if (arch_selection == 'B' || arch_selection == 'b') detected_arch = "arm64";
                else {
                    printf("Invalid architecture selection.\n");
                    return;
                }

                sprintf(url_sel, "https://github.com/mxp96/pawncc-termux/releases/download/v%s/pawncc-%s-%s.zip",
                        termux_versions[pcc_sel_index], termux_versions[pcc_sel_index], detected_arch);
                sprintf(fname_sel, "pawncc-%s-%s.zip", termux_versions[pcc_sel_index], detected_arch);

                wcfg.ipcc = 1;
                watchdogs_download_file(url_sel, fname_sel);
                return;
            }
        }

        const char *list_versions[] = {
            "3.10.11", "3.10.10", "3.10.9", "3.10.8", "3.10.7",
            "3.10.6", "3.10.5", "3.10.4", "3.10.3", "3.10.2"
        };

        printf("Select the PawnCC version to download:\n");
        for (int i = 0; i < 10; i++)
            printf("[%c/%c] PawnCC %s\n", 'A'+i, 'a'+i, list_versions[i]);

        printf(">> ");
        if (scanf(" %c", &version_selection) != 1) return;

        int pcc_sel_index = (version_selection >= 'A' && version_selection <= 'J')
            ? version_selection - 'A'
            : (version_selection >= 'a' && version_selection <= 'j')
            ? version_selection - 'a'
            : -1;

        if (pcc_sel_index < 0 || pcc_sel_index >= 10) {
            printf("Invalid selection.\n");
            return;
        }

        const char *pcc_archive_ext, *pcc_base_repo;
        if (strcmp(list_versions[pcc_sel_index], "3.10.11") == 0)
            pcc_base_repo = "https://github.com/openmultiplayer/compiler";
        else
            pcc_base_repo = "https://github.com/pawn-lang/compiler";

        if (strcmp(platform, "linux") == 0)
            pcc_archive_ext = "tar.gz";
        else if (strcmp(platform, "windows") == 0)
            pcc_archive_ext = "zip";
        else {
            printf("Unknown platform: %s\n", platform);
            return;
        }

        sprintf(url_sel, "%s/releases/download/v%s/pawnc-%s-%s.%s",
                pcc_base_repo, list_versions[pcc_sel_index], list_versions[pcc_sel_index], platform, pcc_archive_ext);
        sprintf(fname_sel, "pawnc-%s-%s.%s", list_versions[pcc_sel_index], platform, pcc_archive_ext);

        wcfg.ipcc = 1;
        watchdogs_download_file(url_sel, fname_sel);
}

void
watch_samp(const char *platform) {
        VersionInfo list_versions[] = {
            { 'A', "SA-MP 0.3.DL R1", 
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp03DLsvr_R1.tar.gz",
                "samp03DLsvr_R1.tar.gz",
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp03DL_svr_R1_win32.zip",
                "samp03DL_svr_R1_win32.zip"
            },
            { 'B', "SA-MP 0.3.7 R3", 
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R3.tar.gz",
                "samp037svr_R3.tar.gz",
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R3_win32.zip",
                "samp037_svr_R3_win32.zip"
            },
            { 'C', "SA-MP 0.3.7 R2-2-1", 
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-2-1.tar.gz",
                "samp037svr_R2-2-1.tar.gz",
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
                "samp037_svr_R2-2-1_win32.zip"
            },
            { 'D', "SA-MP 0.3.7 R2-1-1", 
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037svr_R2-1.tar.gz",
                "samp037svr_R2-1.tar.gz",
                "https://github.com/KrustyKoyle/files.sa-mp.com-Archive/raw/refs/heads/master/samp037_svr_R2-1-1_win32.zip",
                "samp037_svr_R2-1-1_win32.zip"
            },
            { 'E', "OPEN.MP v1.4.0.2779",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.4.0.2779/open.mp-linux-x86.tar.gz",
                "open.mp-linux-x86.tar.gz",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.4.0.2779/open.mp-win-x86.zip",
                "open.mp-win-x86.zip"
            },
            { 'F', "OPEN.MP v1.3.1.2748",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.3.1.2748/open.mp-linux-x86.tar.gz",
                "open.mp-linux-x86.tar.gz",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.3.1.2748/open.mp-win-x86.zip",
                "open.mp-win-x86.zip"
            },
            { 'G', "OPEN.MP v1.2.0.2670",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.2.0.2670/open.mp-linux-x86.tar.gz",
                "open.mp-linux-x86.tar.gz",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.2.0.2670/open.mp-win-x86.zip",
                "open.mp-win-x86.zip"
            },
            { 'H', "OPEN.MP v1.1.0.2612",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.1.0.2612/open.mp-linux-x86.tar.gz",
                "open.mp-linux-x86.tar.gz",
                "https://github.com/openmultiplayer/open.mp/releases/download/v1.1.0.2612/open.mp-win-x86.zip",
                "open.mp-win-x86.zip"
            }
        };

        printf("Select the SA-MP version to download:\n");
        for (int i = 0; i < sizeof(list_versions)/sizeof(list_versions[0]); i++) {
            printf("[%c/%c] %s\n", list_versions[i].key, list_versions[i].key + 32, list_versions[i].name);
        }

        printf(">> ");
        char version_choice;
        if (scanf(" %c", &version_choice) != 1) { return; }

        VersionInfo *chosen_selection = NULL;
        for (int i = 0; i < sizeof(list_versions)/sizeof(list_versions[0]); i++) {
            if (version_choice == list_versions[i].key ||
                version_choice == list_versions[i].key + 32) {
                chosen_selection = &list_versions[i];
                break;
            }
        }

        if (!chosen_selection) {
            printf("Invalid selection!\n");
            return;
        } else {
            const char
            	*url_sel = strcmp(platform, "linux") == 0 ? chosen_selection->linux_url : chosen_selection->windows_url;
            const char
            	*fname_sel = strcmp(platform, "linux") == 0 ? chosen_selection->linux_file : chosen_selection->windows_file;
            watchdogs_download_file(url_sel, fname_sel);
        }
}
