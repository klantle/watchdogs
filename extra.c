#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <shlwapi.h>
#include <strings.h>
#include <io.h>
#define __PATH_SYM "\\"
#define IS_PATH_SYM(c) ((c) == '/' || (c) == '\\')
#define mkdir(path) _mkdir(path)
#define sleep(sec) Sleep((sec)*1000)
#define setenv(name,val,overwrite) _putenv_s(name,val)
static int _w_chmod(const char *path) {
        int mode = _S_IREAD | _S_IWRITE;
        return chmod(path, mode);
}
#else
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#define __PATH_SYM "/"
#define IS_PATH_SYM(c) ((c) == '/')
#endif

#include "color.h"
#include "chain.h"
#include "utils.h"
#include "extra.h"

void println(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        printf("\n");
        va_end(args);
}

void printf_color(const char *color,
                  const char *format, ...)
{
        va_list args;
        va_start(args, format);

        printf("%s", color);
        vprintf(format, args);
        printf("%s", COL_DEFAULT);

        va_end(args);
}

void printf_succes(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_YELLOW, "succes: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_info(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_YELLOW, "info: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_warning(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_GREEN, "warning: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_RED, "error: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void printf_crit(const char *format, ...) {
        va_list args;
        va_start(args, format);
        printf_color(COL_RED, "crit: ");
        vprintf(format, args);
        printf("\n");
        va_end(args);
}

void
wd_ApplyPawncc(void) {        
        int __find_pawncc_exe = 0,
            __find_pawncc = 0,
            __find_pawndisasm_exe = 0,
            __find_pawndisasm = 0;

        int dir_pawno=0, dir_qawno=0;
        char *dest_path = NULL, str_dest_path[128];

        char pawncc_dest_path[PATH_MAX] = {0},
             pawncc_exe_dest_path[PATH_MAX] = {0},
             pawndisasm_dest_path[PATH_MAX] = {0},
             pawndisasm_exe_dest_path[PATH_MAX] = {0};

        wd_sef_fdir_reset();
        
        if (wcfg.f_samp == 0x01)
        {
__def:
            __find_pawncc_exe = wd_sef_fdir(".", "pawncc.exe", "pawno"),
            __find_pawncc = wd_sef_fdir(".", "pawncc", "pawno"),
            __find_pawndisasm_exe = wd_sef_fdir(".", "pawndisasm.exe", "pawno"),
            __find_pawndisasm = wd_sef_fdir(".", "pawndisasm", "pawno");
        } else if (wcfg.f_openmp == 0x01) {
            __find_pawncc_exe = wd_sef_fdir(".", "pawncc.exe", "qawno"),
            __find_pawncc = wd_sef_fdir(".", "pawncc", "qawno"),
            __find_pawndisasm_exe = wd_sef_fdir(".", "pawndisasm.exe", "qawno"),
            __find_pawndisasm = wd_sef_fdir(".", "pawndisasm", "qawno");
        } else { goto __def; }

        struct stat st;
        if (stat("pawno", &st) == 0 && S_ISDIR(st.st_mode)) {
                dir_pawno=1;
                dest_path="pawno";
        }
        if (stat("qawno", &st) == 0 && S_ISDIR(st.st_mode)) {
                dir_qawno=1;
                dest_path="qawno";
        }
        if (!dir_pawno && !dir_qawno) {
#ifdef _WIN32
            if (mkdir("pawno") == 0) dest_path = "pawno";
#else
            if (mkdir("pawno", 0755) == 0) dest_path = "pawno";
#endif
        }

        for (int i = 0; i < wcfg.sef_count; i++) {
            const char *__sef_entry = wcfg.sef_found[i];
            if (!__sef_entry) continue;

            if (strstr(__sef_entry, "pawncc.exe")) {
                snprintf(pawncc_exe_dest_path, sizeof(pawncc_exe_dest_path), "%s", __sef_entry);
                __find_pawncc_exe = 1;
            } else if (strstr(__sef_entry, "pawncc")) {
                snprintf(pawncc_dest_path, sizeof(pawncc_dest_path), "%s", __sef_entry);
                __find_pawncc = 1;
            }
            else if (strstr(__sef_entry, "pawndisasm.exe")) {
                snprintf(pawndisasm_exe_dest_path, sizeof(pawndisasm_exe_dest_path), "%s", __sef_entry);
                __find_pawndisasm_exe = 1;
            } else if (strstr(__sef_entry, "pawndisasm")) {
                snprintf(pawndisasm_dest_path, sizeof(pawndisasm_dest_path), "%s", __sef_entry);
                __find_pawndisasm = 1;
            }
        }

        if (__find_pawncc_exe) {
            snprintf(str_dest_path, sizeof(str_dest_path),
                                    "%s%s%s",
            					    dest_path,
            						"/",
            					    "pawncc.exe");
            wd_sef_wcopy(pawncc_exe_dest_path, str_dest_path);
        }
        if (__find_pawncc) {
            snprintf(str_dest_path, sizeof(str_dest_path),
                                    "%s%s%s",
            						dest_path,
            						"/",
            						"pawncc");
            wd_sef_wcopy(pawncc_dest_path, str_dest_path);
        }
        if (__find_pawndisasm_exe) {
            snprintf(str_dest_path, sizeof(str_dest_path),
                                    "%s%s%s",
            						dest_path,
            						"/",
            						"pawndisasm.exe");
            wd_sef_wcopy(pawndisasm_exe_dest_path, str_dest_path);
        }
        if (__find_pawndisasm) {
            snprintf(str_dest_path, sizeof(str_dest_path),
                                    "%s%s%s",
            						dest_path,
            						"/",
            						"pawndisasm");
            wd_sef_wcopy(pawndisasm_dest_path, str_dest_path);
        }

#ifndef _WIN32
        if (wcfg.__os__ == 0x00) {
                int __find_libpawnc = wd_sef_fdir(".", "libpawnc.so", NULL);
                char libpawnc_dest[PATH_MAX];
                for (int i = 0; i < wcfg.sef_count; i++) {
                        if (strstr(wcfg.sef_found[i], "libpawnc.so")) {
                                snprintf(libpawnc_dest, sizeof(libpawnc_dest),
                                                        "%s",
                                                        wcfg.sef_found[i]);
                                break;
                        }
                }

                struct stat st;
                int lib_or_lib32 = 0;
                char *lib_32_dest = "/usr/local/lib32",
                     *lib_path_dest = "/usr/local/lib",
                     *tx_local_lib_dest = "/data/data/com.termux/files/usr/local/lib/",
                     *tx_lib_path_dest = "/data/data/com.termux/files/usr/lib/";

                char *str_lib_path = NULL;
                char str_full_dest_path[PATH_MAX];

                if (!stat(tx_lib_path_dest, &st) && S_ISDIR(st.st_mode)) {
                    str_lib_path = tx_lib_path_dest;
                } else if (!stat(tx_local_lib_dest, &st) && S_ISDIR(st.st_mode)) {
                    str_lib_path = tx_local_lib_dest;
                } else if (!stat(lib_path_dest, &st) && S_ISDIR(st.st_mode)) {
                    str_lib_path = lib_path_dest;
                    lib_or_lib32 = 0;
                } else if (!stat(lib_32_dest, &st) && S_ISDIR(st.st_mode)) {
                    str_lib_path = lib_32_dest;
                    lib_or_lib32 = 1;
                } else {
                    fprintf(stderr, "No valid library path found!\n");
                    goto __error;
                }

                if (__find_libpawnc == 1) {
                        snprintf(str_full_dest_path, sizeof(str_full_dest_path), "%s/libpawnc.so", str_lib_path);
                        wd_sef_wmv(libpawnc_dest, str_full_dest_path);
                }

                if (strcmp(str_lib_path, lib_path_dest) == 0) {
                        int sys_sudo = wd_RunCommand("which sudo > /dev/null 2>&1");
                        if (sys_sudo == 0)
                            wd_RunCommand("sudo ldconfig");
                        else
                            wd_RunCommand("ldconfig");

                        if (lib_or_lib32 == 1) {
                                const char
                                        *old_path_lib = getenv("LD_LIBRARY_PATH");
                                char
                                        new_path_lib[256];
                                if (old_path_lib)
                                    snprintf(new_path_lib, sizeof(new_path_lib),
                                            "%s:%s",
                                            lib_path_dest,
                                            old_path_lib);
                                else snprintf(new_path_lib, sizeof(new_path_lib), "%s", lib_path_dest);
                                setenv("LD_LIBRARY_PATH", new_path_lib, 1);
                        } else if (lib_or_lib32 == 2) {
                                const char
                                        *old_path_lib32 = getenv("LD_LIBRARY_PATH");
                                char
                                        new_path_lib32[256];
                                if (old_path_lib32)
                                    snprintf(new_path_lib32, sizeof(new_path_lib32),
                                            "%s:%s",
                                            lib_32_dest,
                                            old_path_lib32);
                                else snprintf(new_path_lib32, sizeof(new_path_lib32), "%s", lib_32_dest);
                                setenv("LD_LIBRARY_PATH", new_path_lib32, 1);
                        }
                } else if (strcmp(str_lib_path, tx_local_lib_dest) == 0) {
                        const char
                                *old_path_lib_tr = getenv("LD_LIBRARY_PATH");
                        char
                                new_path_lib_tx[256];
                        if (old_path_lib_tr)
                            snprintf(new_path_lib_tx, sizeof(new_path_lib_tx),
                                    "%s:%s",
                                    tx_local_lib_dest,
                                    old_path_lib_tr);
                        else snprintf(new_path_lib_tx, sizeof(new_path_lib_tx), "%s", tx_local_lib_dest);
                        setenv("LD_LIBRARY_PATH", new_path_lib_tx, 1);
                } else if (strcmp(str_lib_path, tx_lib_path_dest) == 0) {
                        const char
                                *old_path_lib_tr = getenv("LD_LIBRARY_PATH");
                        char
                                new_path_lib_tx[256];
                        if (old_path_lib_tr)
                            snprintf(new_path_lib_tx, sizeof(new_path_lib_tx),
                                    "%s:%s",
                                    tx_lib_path_dest,
                                    old_path_lib_tr);
                        else snprintf(new_path_lib_tx, sizeof(new_path_lib_tx), "%s", tx_lib_path_dest);
                        setenv("LD_LIBRARY_PATH", new_path_lib_tx, 1);
                } else {}
        }
#endif
__error:
        printf_color(COL_YELLOW, "apply finished!\n");
        __main(0);
}

