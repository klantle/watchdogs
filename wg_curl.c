#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "wg_extra.h"
#include "wg_crypto.h"
#include "wg_archive.h"
#include "wg_depends.h"
#include "wg_compiler.h"
#include "wg_util.h"
#include "wg_unit.h"
#include "wg_curl.h"

const char* AIO_FETCH_SITES[MAX_NUM_SITES][2] = {
	    {"github", "https://github.com/%s"},
	    {"gitlab", "https://gitlab.com/%s"},
	    {"instagram", "https://instagram.com/%s"},
	    {"tiktok", "https://www.tiktok.com/@%s"},
	    {"x", "https://x.com/%s"},
	    {"facebook", "https://www.facebook.com/%s"},
	    {"linkedin", "https://www.linkedin.com/in/%s"},
	    {"reddit", "https://www.reddit.com/user/%s"},
	    {"pinterest", "https://www.pinterest.com/%s"},
	    {"snapchat", "https://www.snapchat.com/add/%s"},
	    {"telegram", "https://t.me/%s"},
	    {"spotify", "https://open.spotify.com/user/%s"},
	    {"soundcloud", "https://soundcloud.com/%s"},
	    {"youtube", "https://www.youtube.com/@%s"},
	    {"stackoverflow", "https://stackoverflow.com/users/%s"},
	    {"devto", "https://dev.to/%s"},
	    {"bitbucket", "https://bitbucket.org/%s"},
	    {"sourceforge", "https://sourceforge.net/u/%s"},
	    {"figma", "https://www.figma.com/@%s"},
	    {"steam", "https://steamcommunity.com/id/%s"},
	    {"twitch", "https://www.twitch.tv/%s"},
	    {"epicgames", "https://www.epicgames.com/id/%s"},
	    {"codepen", "https://codepen.io/%s"},
	    {"jsfiddle", "https://jsfiddle.net/user/%s"},
	    {"replit", "https://replit.com/@%s"},
	    {"hackerrank", "https://www.hackerrank.com/%s"},
	    {"leetcode", "https://leetcode.com/%s"},
	    {"codewars", "https://www.codewars.com/users/%s"},
	    {"kaggle", "https://www.kaggle.com/%s"},
	    {"npm", "https://www.npmjs.com/~%s"},
	    {"docker", "https://hub.docker.com/u/%s"},
	    {"behance", "https://www.behance.net/%s"},
	    {"dribbble", "https://dribbble.com/%s"},
	    {"deviantart", "https://www.deviantart.com/%s"},
	    {"artstation", "https://www.artstation.com/%s"},
	    {"medium", "https://medium.com/@%s"},
	    {"substack", "https://%s.substack.com"},
	    {"wordpress", "https://%s.wordpress.com"},
	    {"blogger", "https://%s.blogspot.com"},
	    {"tumblr", "https://%s.tumblr.com"},
	    {"discord", "https://discord.com/users/%s"},
	    {"signal", "https://signal.me/#p/%s"},
	    {"mastodon", "https://mastodon.social/@%s"},
	    {"bluesky", "https://bsky.app/profile/%s"},
	    {"threads", "https://www.threads.net/@%s"},
	    {"angellist", "https://angel.co/u/%s"},
	    {"crunchbase", "https://www.crunchbase.com/person/%s"},
	    {"slideshare", "https://www.slideshare.net/%s"},
	    {"speakerdeck", "https://speakerdeck.com/%s"},
	    {"etsy", "https://www.etsy.com/shop/%s"},
	    {"shopify", "https://%s.myshopify.com"},
	    {"ebay", "https://www.ebay.com/usr/%s"},
	    {"discord", "https://discord.gg/%s"},
	    {"minecraft", "https://namemc.com/profile/%s"},
	    {"roblox", "https://www.roblox.com/user.aspx?username=%s"},
	    {"origin", "https://www.origin.com/usa/en-us/profile/%s"},
	    {"ubisoft", "https://ubisoftconnect.com/en-US/profile/%s"},
	    {"bandcamp", "https://%s.bandcamp.com"},
	    {"mixcloud", "https://www.mixcloud.com/%s"},
	    {"lastfm", "https://www.last.fm/user/%s"},
	    {"vimeo", "https://vimeo.com/%s"},
	    {"dailymotion", "https://www.dailymotion.com/%s"},
	    {"odysee", "https://odysee.com/@%s"},
	    {"coursera", "https://www.coursera.org/user/%s"},
	    {"udemy", "https://www.udemy.com/user/%s"},
	    {"skillshare", "https://www.skillshare.com/profile/%s"},
	    {"khanacademy", "https://www.khanacademy.org/profile/%s"},
	    {"keybase", "https://keybase.io/%s"},
	    {"gravatar", "https://gravatar.com/%s"},
	    {"flickr", "https://www.flickr.com/people/%s"},
	    {"goodreads", "https://www.goodreads.com/%s"},
	    {"letterboxd", "https://letterboxd.com/%s"},
	    {"trello", "https://trello.com/%s"},
	    {"linktree", "https://linktr.ee/%s"},
	    {"cashapp", "https://cash.app/$%s"},
	    {"venmo", "https://venmo.com/%s"},
	    {"paypal", "https://paypal.me/%s"}
};

static char
	pawncc_dir_src[WG_PATH_MAX];

void verify_cacert_pem(CURL *curl) {
	    int is_win32 = 0;
#ifdef WG_WINDOWS
	    is_win32 = 1;
#endif

	    int is_android = 0;
#ifdef WG_ANDROID
	    is_android = 2;
#endif

	    int is_linux = 0;
#ifdef WG_LINUX
	    is_linux = 3;
#endif

	    static int cacert_notice = 0;

	    if (is_win32) {
	        if (access("cacert.pem", F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
	        else if (access("C:/libwatchdogs/cacert.pem", F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, "C:/libwatchdogs/cacert.pem");
	        else {
	            if (cacert_notice != 1) {
	                cacert_notice = 1;
	                pr_color(stdout,
	                         FCOLOUR_YELLOW,
	                         "curl: i can't found cacert.pem. SSL verification may fail.\n");
	            }
	        }
	    }
	    else if (is_android) {
	        const char *env_home = getenv("HOME");
	        if (env_home == NULL || env_home[0] == '\0') {
	        	env_home = "/data/data/com.termux/files/home";
	        }
	        char size_env_home[WG_PATH_MAX + 6];

	        wg_snprintf(size_env_home, sizeof(size_env_home), "%s/cacert.pem", env_home);

	        if (access("cacert.pem", F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
	        else if (access(size_env_home, F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, size_env_home);
	        else {
	            if (cacert_notice != 1) {
	                cacert_notice = 1;
	                pr_color(stdout,
	                         FCOLOUR_YELLOW,
	                         "curl: i can't found cacert.pem. SSL verification may fail.\n");
	            }
	        }
	    }
	    else if (is_linux) {
	        if (access("cacert.pem", F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, "cacert.pem");
	        else if (access("/etc/ssl/certs/cacert.pem", F_OK) == WG_RETZ)
	            curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/cacert.pem");
	        else {
	            if (cacert_notice != 1) {
	                cacert_notice = 1;
	                pr_color(stdout,
	                         FCOLOUR_GREEN,
	                         "curl: i can't found cacert.pem. SSL verification may fail.\n");
	            }
	        }
	    }
}

static int progress_callback(void *ptr, curl_off_t dltotal,
                             curl_off_t dlnow, curl_off_t ultotal,
                             curl_off_t ulnow)
{
		static int last_percent = -1;
		static int dot_index = 0;
		static const char term_spinner[] = "-\\|/";

		int percent;

		if (dltotal > 0) {
			percent = (int)((dlnow * 100) / dltotal);

			if (percent != last_percent) {
				char spin_char = term_spinner[dot_index % 4];
				dot_index++;

				if (percent < 100)
					printf("\rDownloading... %3d%% [%c]", percent, spin_char);
				else
					printf("\rDownloading... %3d%% Done!\n", percent);

				fflush(stdout);
				last_percent = percent;
			}
		}

		return WG_RETZ;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
		size_t total = size * nmemb;
		struct buf *b = userdata;
		
		if (total > 0xFFFFFFF) return WG_RETZ;
		
		size_t required = b->len + total + 1;
		if (required > b->allocated) {
			size_t new_alloc = (b->allocated * 3) >> 1;
			new_alloc = (required > new_alloc) ? required : new_alloc;
			new_alloc = (new_alloc < 0x4000000) ? new_alloc : 0x4000000;
			
			char *p = wg_realloc(b->data, new_alloc);
			if (!p) return WG_RETZ;
			
			b->data = p;
			b->allocated = new_alloc;
		}
		
		memcpy(b->data + b->len, ptr, total);
		b->len += total;
		b->data[b->len] = 0;
		
		return total;
}

size_t write_callback_sites(void *data, size_t size, size_t nmemb, void *userp) 
{
		size_t total_size = size * nmemb;
		struct string *str = (struct string *)userp;
		
		if (total_size > 0x8000000) {
#if defined(_DBG_PRINT)
        	pr_error(stdout, "Oversized chunk: 0x%zX\n", total_size);
#endif
			return WG_RETZ;
		}
		
		size_t new_capacity = str->len + total_size + 1;
		if (new_capacity > str->allocated) {
			size_t growth = str->allocated + (str->allocated >> 1);
			new_capacity = (new_capacity > growth) ? new_capacity : growth;
			new_capacity = (new_capacity < 0x2000000) ? new_capacity : 0x2000000;
			
			char *new_ptr = wg_realloc(str->ptr, new_capacity);
			if (!new_ptr) {
#if defined(_DBG_PRINT)
            	pr_error(stdout, "Allocation failed for 0x%zX bytes\n", new_capacity);
#endif
				return WG_RETZ;
			}
			str->ptr = new_ptr;
			str->allocated = new_capacity;
		}
		
		memcpy(str->ptr + str->len, data, total_size);
		str->len += total_size;
		str->ptr[str->len] = '\0';
		
		return total_size;
}

size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
		struct memory_struct *mem = userp;
		size_t realsize = size * nmemb;
		
		if (!contents || !mem || realsize > 0x10000000)
			return WG_RETZ;
		
		size_t required = mem->size + realsize + 1;
		if (required > mem->allocated) {
			size_t new_alloc = mem->allocated ? (mem->allocated * 2) : 0x1000;
			new_alloc = (required > new_alloc) ? required : new_alloc;
			new_alloc = (new_alloc < 0x8000000) ? new_alloc : 0x8000000;
			
			char *ptr = wg_realloc(mem->memory, new_alloc);
			if (!ptr) {
#if defined(_DBG_PRINT)
            	pr_error(stdout, "Memory exhausted at 0x%zX bytes\n", new_alloc);
#endif
				return WG_RETZ;
			}
			mem->memory = ptr;
			mem->allocated = new_alloc;
    }
		
		memcpy(&mem->memory[mem->size], contents, realsize);
		mem->size += realsize;
		mem->memory[mem->size] = '\0';
		
		return realsize;
}

void
account_tracker_discrepancy(const char *base, char discrepancy[][100], int *cnt) {
	    int len = strlen(base);
	    int i;

	    wg_strcpy(discrepancy[(*cnt)++], base);

	    for (i = 0; i < len; i++) {
	        char size_temp[100];
	        strncpy(size_temp, base, i);
	        size_temp[i] = base[i];
	        size_temp[i+1] = base[i];
	        wg_strcpy(size_temp + i + 2, base + i + 1);
	        wg_strcpy(discrepancy[(*cnt)++], size_temp);
	    }

	    for (i = 2; i <= 5; i++) {
	        char size_temp[100];
	        wg_snprintf(size_temp, sizeof(size_temp), "%s", base);
	        for (int j = 0; j < i; j++) {
	            size_temp[strlen(size_temp)] = base[len - 1];
	        }
	        wg_strcpy(discrepancy[(*cnt)++], size_temp);
	    }

	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "1"));
	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "123"));
	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "007"));
	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "_"));
	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "."));
	    wg_strcpy(discrepancy[(*cnt)++], strcat(strdup(base), "_dev"));
}

void account_tracking_username(CURL *curl, const char *username) {
	    CURLcode res;
	    struct string response;
	    response.ptr = wg_malloc(1);
	    response.len = 0;

	    struct curl_slist *headers = NULL;
	    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0");

	    for (int i = 0; i < MAX_NUM_SITES; i++) {
	        char url[200];
	        wg_snprintf(url, sizeof(url), AIO_FETCH_SITES[i][1], username);

	        curl_easy_setopt(curl, CURLOPT_URL, url);
	        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_sites);
	        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);

	        verify_cacert_pem(curl);

	        res = curl_easy_perform(curl);

	        if (res != CURLE_OK) {
	            printf("[%s] %s -> ERROR %s\n", AIO_FETCH_SITES[i][0], url, curl_easy_strerror(res));
	        } else {
	            long status_code;
	            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
	            if (status_code == WG_CURL_RESPONSE_OK) {
	                printf("[%s] %s -> FOUND\n", AIO_FETCH_SITES[i][0], url);
	                fflush(stdout);
	            } else {
	                printf("[%s] %s -> NOT FOUND (%ld)\n", AIO_FETCH_SITES[i][0], url, status_code);
	                fflush(stdout);
	            }
	        }
	        response.len = 0;
	    }
}

static void find_compiler_tools(int compiler_type,
								int *found_pawncc_exe, int *found_pawncc,
								int *found_pawndisasm_exe, int *found_pawndisasm,
								int *found_pawnc_dll, int *found_PAWNC_DLL)
{
		const char *ignore_dir = NULL;
		char size_pf[WG_PATH_MAX + 56];

		wg_snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);

		*found_pawncc_exe = wg_sef_fdir(size_pf, "pawncc.exe", ignore_dir);
		*found_pawncc = wg_sef_fdir(size_pf, "pawncc", ignore_dir);
		*found_pawndisasm_exe = wg_sef_fdir(size_pf, "pawndisasm.exe", ignore_dir);
		*found_pawndisasm = wg_sef_fdir(size_pf, "pawndisasm", ignore_dir);
		*found_pawnc_dll = wg_sef_fdir(size_pf, "pawnc.dll", ignore_dir);
		*found_PAWNC_DLL = wg_sef_fdir(size_pf, "PAWNC.dll", ignore_dir);
}

static const char *get_compiler_directory(void)
{
		struct stat st;
		const char *dir_path = NULL;

		if (stat("pawno", &st) == WG_RETZ && S_ISDIR(st.st_mode)) {
				dir_path = "pawno";
		} else if (stat("qawno", &st) == WG_RETZ && S_ISDIR(st.st_mode)) {
				dir_path = "qawno";
		} else {
				if (MKDIR("pawno") == WG_RETZ)
						dir_path = "pawno";
		}

		return dir_path;
}

static void copy_compiler_tool(const char *src_path, const char *tool_name,
						       const char *dest_dir)
{
		char dest_path[WG_PATH_MAX];

		wg_snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, tool_name);
		wg_sef_wmv(src_path, dest_path);
}

#ifndef WG_WINDOWS
static void update_library_environment(const char *lib_path)
{
	    const char *home_dir = getenv("HOME");
	    if (!home_dir) {
	        fprintf(stderr, "Error: HOME environment variable not set\n");
	        return;
	    }

	    const char *shell_rc = NULL;
	    char shell_path[256];

	    char *shell = getenv("SHELL");
	    if (shell) {
	        if (strfind(shell, "zsh")) {
	            shell_rc = ".zshrc";
	        } else if (strfind(shell, "bash")) {
	            shell_rc = ".bashrc";
	        }
	    }

	    if (!shell_rc) {
	        wg_snprintf(shell_path, sizeof(shell_path), "%s/.zshrc", home_dir);
	        if (access(shell_path, F_OK) == WG_RETZ) {
	            shell_rc = ".zshrc";
	        } else {
	            shell_rc = ".bashrc";
	        }
	    }

	    char config_file[256];
	    wg_snprintf(config_file, sizeof(config_file), "%s/%s", home_dir, shell_rc);

	    char grep_cmd[512];
	    wg_snprintf(grep_cmd, sizeof(grep_cmd),
	                "grep -q \"LD_LIBRARY_PATH.*%s\" %s", lib_path, config_file);

	    int path_exists = system(grep_cmd);

	    if (path_exists != 0) {
	        char export_cmd[512];
	        char backup_cmd[WG_PATH_MAX * 3];

	        wg_snprintf(backup_cmd, sizeof(backup_cmd), "cp %s %s.backup", config_file, config_file);
	        system(backup_cmd);

	        wg_snprintf(export_cmd, sizeof(export_cmd),
	                   "echo 'export LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH' >> %s",
	                   lib_path, config_file);

	        int ret = system(export_cmd);

	        if (ret == WG_RETZ) {
	            printf("Successfully updated %s. Please run 'source %s' to apply changes.\n",
	                   shell_rc, config_file);
	        } else {
	            fprintf(stderr, "Error updating %s\n", shell_rc);
	        }
	    } else {
	        printf("Library path already exists in %s\n", shell_rc);
	    }

	    if (strfind(lib_path, "/usr/")) {
	        int is_not_sudo = system("sudo echo > /dev/null");
	        if (is_not_sudo == WG_RETZ) {
	            system("sudo ldconfig");
	        } else {
	            system("ldconfig");
	        }
	    }
}
#endif

static int setup_linux_library(void)
{
#ifdef WG_WINDOWS
		return WG_RETZ;
#else
		const char *selected_path = NULL;
		char libpawnc_src[WG_PATH_MAX];
		char dest_path[WG_PATH_MAX];
		struct stat st;
		int i, found_lib;

		const char *lib_paths[] =	{
										"/data/data/com.termux/files/usr/lib/",
										"/data/data/com.termux/files/usr/local/lib/",
										"/data/data/com.termux/arm64/usr/lib",
										"/data/data/com.termux/arm32/usr/lib",
										"/data/data/com.termux/amd32/usr/lib",
										"/data/data/com.termux/amd64/usr/lib",
										"/usr/local/lib",
										"/usr/local/lib32"
									};

		if (!strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_WINDOWS) ||
			!strcmp(wgconfig.wg_toml_os_type, OS_SIGNAL_UNKNOWN))
				return WG_RETZ;

		char size_pf[WG_PATH_MAX + 56];
		wg_snprintf(size_pf, sizeof(size_pf), "%s", pawncc_dir_src);
		found_lib = wg_sef_fdir(size_pf, "libpawnc.so", NULL);
		if (!found_lib)
				return WG_RETZ;

		for (i = 0; i < wgconfig.wg_sef_count; i++) {
				if (strstr(wgconfig.wg_sef_found_list[i], "libpawnc.so")) {
						wg_strncpy(libpawnc_src, wgconfig.wg_sef_found_list[i], sizeof(libpawnc_src));
						break;
				}
		}

		for (i = 0; i < sizeof(lib_paths) / sizeof(lib_paths[0]); i++) {
				if (stat(lib_paths[i], &st) == WG_RETZ && S_ISDIR(st.st_mode)) {
						selected_path = lib_paths[i];
						break;
				}
		}

		if (!selected_path) {
				fprintf(stderr, "No valid library path found!\n");
				return -WG_RETN;
		}

		wg_snprintf(dest_path, sizeof(dest_path), "%s/libpawnc.so", selected_path);
		wg_sef_wmv(libpawnc_src, dest_path);

		update_library_environment(selected_path);

		return WG_RETZ;
#endif
}

void wg_apply_pawncc(void)
{
		int found_pawncc_exe, found_pawncc;
		int found_pawndisasm_exe, found_pawndisasm;
		int found_pawnc_dll, found_PAWNC_DLL;

		const char *dest_dir;

		char pawncc_src[WG_PATH_MAX] = { 0 },
			 pawncc_exe_src[WG_PATH_MAX] = { 0 },
			 pawndisasm_src[WG_PATH_MAX] = { 0 },
			 pawndisasm_exe_src[WG_PATH_MAX] = { 0 },
			 pawnc_dll_src[WG_PATH_MAX] = { 0 },
			 PAWNC_DLL_src[WG_PATH_MAX] = { 0 };

		int i;

		wg_sef_fdir_reset();

		find_compiler_tools(wgconfig.wg_is_samp ? COMPILER_SAMP : COMPILER_OPENMP,
						    &found_pawncc_exe, &found_pawncc,
						    &found_pawndisasm_exe, &found_pawndisasm,
							&found_pawnc_dll, &found_PAWNC_DLL);

		dest_dir = get_compiler_directory();
		if (!dest_dir) {
				pr_error(stdout, "Failed to create compiler directory");
				goto apply_done;
		}

		for (i = 0; i < wgconfig.wg_sef_count; i++) {
				const char *item = wgconfig.wg_sef_found_list[i];
				if (!item)
						continue;

				if (strstr(item, "pawncc.exe")) {
						wg_strncpy(pawncc_exe_src, item, sizeof(pawncc_exe_src));
				} else if (strstr(item, "pawncc") && !strstr(item, ".exe")) {
						wg_strncpy(pawncc_src, item, sizeof(pawncc_src));
				} else if (strstr(item, "pawndisasm.exe")) {
						wg_strncpy(pawndisasm_exe_src, item, sizeof(pawndisasm_exe_src));
				} else if (strstr(item, "pawndisasm") && !strstr(item, ".exe")) {
						wg_strncpy(pawndisasm_src, item, sizeof(pawndisasm_src));
				} else if (strstr(item, "pawnc.dll"))  {
						wg_strncpy(pawnc_dll_src, item, sizeof(pawnc_dll_src));
				} else if (strstr(item, "PAWNC.dll")) {
						wg_strncpy(PAWNC_DLL_src, item, sizeof(PAWNC_DLL_src));
				}
		}

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

		setup_linux_library();

		char rm_cmd[WG_PATH_MAX + 56];
		if (is_native_windows())
			wg_snprintf(rm_cmd, sizeof(rm_cmd),
				"rmdir /s /q \"%s\"",
				pawncc_dir_src);
		else
			wg_snprintf(rm_cmd, sizeof(rm_cmd),
				"rm -rf %s",
				pawncc_dir_src);
		wg_run_command(rm_cmd);

		memset(pawncc_dir_src, 0, sizeof(pawncc_dir_src));

		pr_info(stdout, "Compiler installed successfully!");

		pr_color(stdout, FCOLOUR_CYAN, "Testing our compiler? [Y/n]");
		char *compile_now = readline(" ");
		if (!strcmp(compile_now, "Y") || !strcmp(compile_now, "y")) {
			wg_free(compile_now);
          	pr_color(stdout, FCOLOUR_CYAN, "~ input pawn files (press enter for from config toml - enter E/e to exit):");
			char *gamemode_compile = readline(" ");
			if (strlen(gamemode_compile) < 1) {
		            const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		            wg_run_compiler(args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
					wg_free(gamemode_compile);
			} else {
		            const char *args[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		            wg_run_compiler(args[0], gamemode_compile, args[2], args[3], args[4], args[5], args[6], args[7], args[8]);
					wg_free(gamemode_compile);
			}
		} else { wg_free(compile_now); }

apply_done:
		chain_goto_main(NULL);
}

static int prompt_apply_pawncc(void)
{
		wgconfig.wg_ipawncc = 0;

		pr_color(stdout, FCOLOUR_YELLOW, "Apply pawncc?");
		char *confirm = readline(" [y/n]: ");
		fflush(stdout);

		if (!confirm) {
			fprintf(stderr, "Error reading input\n");
			wg_free(confirm);
			goto done;
		}

		if (strlen(confirm) == WG_RETZ) {
			wg_free(confirm);
			confirm = readline(" >>> [y/n]: ");
		}

		if (confirm) {
			if (strcmp(confirm, "Y") == WG_RETZ || strcmp(confirm, "y") == WG_RETZ) {
				wg_free(confirm);
				return WG_RETN;
			}
		}

done:
		return WG_RETZ;
}

int is_archive_file(const char *filename)
{
		if (strfind(filename, ".tar") ||
			strfind(filename, ".zip"))
			return WG_RETN;
		return WG_RETZ;
}

static int debug_callback(CURL *handle, curl_infotype type,
                          char *data, size_t size, void *userptr)
{
	    (void)handle;
	    (void)userptr;

	    switch (type) {
	    case CURLINFO_TEXT:
	    case CURLINFO_HEADER_OUT:
	    case CURLINFO_DATA_OUT:
	    case CURLINFO_SSL_DATA_OUT:
	    	break;
	    case CURLINFO_HEADER_IN:
	        if (strfind(data, "location: ") || strfind(data, "content-security-policy: "))
	        	break;
	        printf("<= Recv header: %.*s", (int)size, data);
	        break;
	    case CURLINFO_DATA_IN:
	    case CURLINFO_SSL_DATA_IN:
	    default:
	        break;
	    }
	    return WG_RETZ;
}

int wg_download_file(const char *url, const char *filename)
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
		CURL *curl;
		FILE *c_fp;
		CURLcode res;
		long response_code = 0;
		int retry_count = 0;
		struct stat file_stat;

		pr_color(stdout, FCOLOUR_GREEN, "Downloading: %s", filename);

		do {
			c_fp = fopen(filename, "wb");
			if (!c_fp) {
				pr_color(stdout, FCOLOUR_RED, "\nFailed to open file: %s\n", filename);
				return -WG_RETN;
			}

			curl = curl_easy_init();
			if (!curl) {
				pr_color(stdout, FCOLOUR_RED, "\nFailed to initialize CURL\n");
				fclose(c_fp);
				return -WG_RETN;
			}

			struct curl_slist *headers = NULL;

			if (wgconfig.wg_idepends == WG_RETN) {
				if (strstr(wgconfig.wg_toml_github_tokens, "DO_HERE") ||
						   wgconfig.wg_toml_github_tokens == NULL ||
						   strlen(wgconfig.wg_toml_github_tokens) < 1) {
						pr_color(stdout, FCOLOUR_GREEN, " ~ can't read github token!...\n");
						sleep(1);
				} else {
						char auth_header[512];
						wg_snprintf(auth_header, sizeof(auth_header), "Authorization: token %s", wgconfig.wg_toml_github_tokens);
						headers = curl_slist_append(headers, auth_header);
						pr_color(stdout,
								 FCOLOUR_GREEN,
								 "\tCreate token: %s...\n",
								 wg_masked_text(8, wgconfig.wg_toml_github_tokens));
				}
			}

			headers = curl_slist_append(headers, "User-Agent: watchdogs/1.0");
			headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, c_fp);
			curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
			curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

			static int create_debugging = 0;
			static int always_create_debugging = 0;
			if (create_debugging == WG_RETZ) {
				create_debugging = 1;
				printf("\x1b[32m %% create debugging http?\x1b[0m\n");
				char *debug_http = readline("   answer [y/n]: ");
				if (debug_http) {
					if (debug_http[0] == 'Y' || debug_http[0] == 'y') {
						always_create_debugging = 1;
						curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
						curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
						curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
						fflush(stdout);
					}
				}
			}
			if (always_create_debugging) {
				curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
				curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, debug_callback);
				curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
				fflush(stdout);
			}

			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
			curl_easy_setopt(curl, CURLOPT_XFERINFODATA, NULL);

			verify_cacert_pem(curl);

			fflush(stdout);
			res = curl_easy_perform(curl);
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

			curl_easy_cleanup(curl);
			curl_slist_free_all(headers);

			if (c_fp)
				fclose(c_fp);

			if (response_code == WG_CURL_RESPONSE_OK) {
				if (stat(filename, &file_stat) == WG_RETZ && file_stat.st_size > 0) {
					pr_color(stdout, FCOLOUR_GREEN, "Download successful: %" PRIdMAX " bytes\n", (intmax_t)file_stat.st_size);
					fflush(stdout);

					if (!is_archive_file(filename)) {
						printf("File is not an archive!\n");
						return WG_RETN;
					}
					
					printf("Checking file type for extraction...\n");
					fflush(stdout);
					wg_extract_archive(filename);

					char rm_cmd[WG_PATH_MAX];
					if (wgconfig.wg_idepends == WG_RETN) {
						static int rm_deps = 0;
						if (rm_deps == WG_RETZ) {
							printf("\x1b[32m==> removing" FCOLOUR_CYAN "'%s'\x1b?\x1b[0m\n", filename);
							char *confirm = readline("   answer [y/n]: ");
							if (confirm) {
								if (confirm[0] == 'Y' || confirm[0] == 'y') {
									rm_deps = 1;
									if (is_native_windows())
										wg_snprintf(rm_cmd, sizeof(rm_cmd),
											"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
											"rmdir /s /q \"%s\" 2>nul)",
											filename, filename, filename);
									else
										wg_snprintf(rm_cmd, sizeof(rm_cmd),
											"rm -rf %s",
											filename);
									wg_run_command(rm_cmd);
								}
								wg_free(confirm);
							}
						} else {
							if (is_native_windows())
								wg_snprintf(rm_cmd, sizeof(rm_cmd),
									"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
									"rmdir /s /q \"%s\" 2>nul)",
									filename, filename, filename);
							else
								wg_snprintf(rm_cmd, sizeof(rm_cmd),
									"rm -rf %s",
									filename);
							wg_run_command(rm_cmd);
						}
						return WG_RETZ;
					} else {
						printf("\x1b[32m==> removing" FCOLOUR_CYAN "'%s'\x1b?\x1b[0m\n", filename);
						char *confirm = readline("   answer [y/n]: ");
						if (confirm) {
							if (confirm[0] == 'Y' || confirm[0] == 'y') {
								char rm_cmd[WG_PATH_MAX];
								if (is_native_windows())
									wg_snprintf(rm_cmd, sizeof(rm_cmd),
										"if exist \"%s\" (del /f /q \"%s\" 2>nul || "
										"rmdir /s /q \"%s\" 2>nul)",
										filename, filename, filename);
								else
									wg_snprintf(rm_cmd, sizeof(rm_cmd),
										"rm -rf %s",
										filename);
								wg_run_command(rm_cmd);
							}
							wg_free(confirm);
						}
					}

					if (wgconfig.wg_ipawncc && prompt_apply_pawncc()) {
						char size_filename[WG_PATH_MAX];
						wg_snprintf(size_filename, sizeof(size_filename), "%s", filename);

						if (strstr(size_filename, ".tar.gz")) {
							char *f_EXT = strstr(size_filename, ".tar.gz");
							if (f_EXT)
								*f_EXT = '\0';
						} else {
							char *f_EXT = strrchr(size_filename, '.');
							if (f_EXT)
								*f_EXT = '\0';
						}
						wg_snprintf(pawncc_dir_src, sizeof(pawncc_dir_src), "%s", size_filename);
						wg_apply_pawncc();
					}

					return WG_RETZ;
				} else {
					pr_color(stdout, FCOLOUR_RED, "Downloaded file is empty or missing\n");
					return WG_RETN;
				}
			} else {
				pr_color(stdout, FCOLOUR_RED, "Download failed - "
						 "HTTP: %ld, "
						 "CURL: %d, "
						 "retrying... \n", response_code, res);
			}

			++retry_count;
			sleep(3);
		} while (retry_count < 5);

		pr_color(stdout, FCOLOUR_RED, "Download failed after 5 retries\n");
		chain_goto_main(NULL);

		return WG_RETN;
}