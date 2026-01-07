/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */
#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>
#include  <errno.h>
#include  <time.h>
#include  <dirent.h>
#include  <unistd.h>
#include  <sys/stat.h>
#include  <curl/curl.h>

#include  "extra.h"
#include  "utils.h"
#include  "curl.h"
#include  "archive.h"
#include  "crypto.h"
#include  "units.h"
#include  "debug.h"
#include  "replicate.h"

const  char		*opr = NULL;
static char		 command[DOG_MAX_PATH * 4];
static char		*tag;
static char		*path;
static char		*path_slash;
static char		*user;
static char		*repo;
static char		*repo_slash;
static char		*git_dir;
static char		*filename;
static char		*extension;
static char		 json_item[DOG_PATH_MAX];
static int		 fdir_counts = REPLICATE_RATE_ZERO;

const char		*match_windows_lookup_pattern[] =
	BUILD_PATTERNS("windows", "win", "win32", "win32", "msvc", "mingw");

const char		*match_linux_lookup_pattern[] =
	BUILD_PATTERNS("linux", "ubuntu", "debian", "cent", "centos",
	    "almalinux", "rockylinux", "cent_os", "fedora", "arch",
	    "archlinux", "alphine", "rhel", "redhat", "linuxmint", "mint");

const char		*match_any_lookup_pattern[] =
	BUILD_PATTERNS("src", "source", "proj", "project", "server",
	    "_server", "gamemode", "gamemodes", "bin", "build", "packages",
	    "resources", "modules", "plugins", "addons", "extensions",
	    "scripts", "system", "core", "runtime", "libs", "include",
	    "deps", "dependencies");

int
this_os_archive(const char *filename)
{
	int		 k;
#ifdef DOG_WINDOWS
	opr = "windows";
#else
	opr = "linux";
#endif
	char		 size_host_os[DOG_PATH_MAX];
	char		 filename_lwr[DOG_PATH_MAX];
	const char	**lookup_pattern = NULL;

	strncpy(size_host_os, opr, sizeof(size_host_os) - 1);
	size_host_os[sizeof(size_host_os) - 1] = '\0';

	for (int i = REPLICATE_RATE_ZERO; size_host_os[i]; i++)
		size_host_os[i] = tolower(size_host_os[i]);

	if (strfind(size_host_os, "win", true))
		lookup_pattern = match_windows_lookup_pattern;
	else if (strfind(size_host_os, "linux", true))
		lookup_pattern = match_linux_lookup_pattern;

	if (!lookup_pattern)
		return (0);

	strncpy(filename_lwr, filename, sizeof(filename_lwr) - 1);
	filename_lwr[sizeof(filename_lwr) - 1] = '\0';

	for (int i = REPLICATE_RATE_ZERO; filename_lwr[i]; i++)
		filename_lwr[i] = tolower(filename_lwr[i]);

	for (k = REPLICATE_RATE_ZERO; lookup_pattern[k] != NULL; ++k) {
		if (strfind(filename_lwr, lookup_pattern[k], true))
			return (1);
	}

	return (0);
}

int
this_more_archive(const char *filename)
{
	int	 k;
	int	 ret = REPLICATE_RATE_ZERO;

	for (k = REPLICATE_RATE_ZERO; match_any_lookup_pattern[k] != NULL; ++k) {
		if (strfind(filename, match_any_lookup_pattern[k], true)) {
			ret = 1;
			break;
		}
	}

	return (ret);
}

static char *
try_build_os_asseets(char **assets, int count, const char *os_pattern)
{
	int			 i;
	const char	*const	*lookup_pattern = NULL;

	if (strfind(os_pattern, "win", true))
		lookup_pattern = match_windows_lookup_pattern;
	else if (strfind(os_pattern, "linux", true))
		lookup_pattern = match_linux_lookup_pattern;
	else
		return (NULL);

	for (i = REPLICATE_RATE_ZERO; i < count; i++) {
		const char	*asset = assets[i];
		int		 p;

		for (p = REPLICATE_RATE_ZERO; lookup_pattern[p]; p++) {
			if (!strfind(asset, lookup_pattern[p], true))
				continue;

			return (strdup(asset));
		}
	}

	return (NULL);
}

static char *
try_server_assets(char **assets, int count, const char *os_pattern)
{
	const char	*const	*os_patterns = NULL;
	int			 i;

	if (strfind(os_pattern, "win", true))
		os_patterns = match_windows_lookup_pattern;
	else if (strfind(os_pattern, "linux", true))
		os_patterns = match_linux_lookup_pattern;
	else
		return (NULL);

	for (i = REPLICATE_RATE_ZERO; i < count; i++) {
		const char	*asset = assets[i];
		int		 p;

		for (p = REPLICATE_RATE_ZERO; match_any_lookup_pattern[p]; p++) {
			if (strfind(asset, match_any_lookup_pattern[p], true))
				break;
		}

		if (!match_any_lookup_pattern[p])
			continue;

		for (p = REPLICATE_RATE_ZERO; os_patterns[p]; p++) {
			if (strfind(asset, os_patterns[p], true))
				return (strdup(asset));
		}
	}

	return (NULL);
}

static char *
try_generic_assets(char **assets, int count)
{
	int	i;

	for (i = REPLICATE_RATE_ZERO; i < count; i++) {
		const char	*asset = assets[i];
		int		 p;

		for (p = REPLICATE_RATE_ZERO; match_any_lookup_pattern[p]; p++) {
			if (strfind(asset, match_any_lookup_pattern[p], true))
				return (strdup(asset));
		}
	}

	for (i = REPLICATE_RATE_ZERO; i < count; i++) {
		const char	*asset = assets[i];
		int		 p;

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

		return (strdup(asset));
	}

	return (strdup(assets[0]));
}

char *
package_fetching_assets(char **package_assets, int counts, const char *pf_os)
{
	char		*result = NULL;
	char		 size_host_os[32] = {0};
	const char	*opr = NULL;

	if (counts == REPLICATE_RATE_ZERO)
		return (NULL);
	if (counts == 1)
		return (strdup(package_assets[0]));

	if (pf_os && pf_os[0]) {
		opr = pf_os;
	} else {
#ifdef DOG_WINDOWS
		opr = "windows";
#else
		opr = "linux";
#endif
	}

	if (opr) {
		strncpy(size_host_os, opr, sizeof(size_host_os) - 1);
		for (int j = REPLICATE_RATE_ZERO; size_host_os[j]; j++)
			size_host_os[j] = tolower(size_host_os[j]);
	}

	if (size_host_os[0]) {
		result = try_server_assets(package_assets, counts, size_host_os);
		if (result)
			return (result);

		result = try_build_os_asseets(package_assets, counts, size_host_os);
		if (result)
			return (result);
	}

	return (try_generic_assets(package_assets, counts));
}

static int
package_parse_repo(const char *input, struct _repositories *pkg_dt)
{
	char	*parse_input, *tag_ptr, *path_ptr, *slash, *repo_ptr, *dot_git;
	char	*choice;

	memset(pkg_dt, 0, sizeof(*pkg_dt));
	parse_input = dog_malloc(1024);
	if (!parse_input)
		return (0);
	strncpy(parse_input, input, 1024 - 1);
	parse_input[1024 - 1] = '\0';

	tag_ptr = strrchr(parse_input, '?');
	if (tag_ptr) {
		*tag_ptr = '\0';
		strncpy(pkg_dt->tag, tag_ptr + 1,
		    sizeof(pkg_dt->tag) - 1);
	}

	path_ptr = parse_input;
	if (strncmp(path_ptr, "https://", 8) == 0)
		path_ptr += 8;
	else if (strncmp(path_ptr, "http://", 7) == 0)
		path_ptr += 7;

	if (strstr(path_ptr, "github.com")) {
		strcpy(pkg_dt->host, "github");
		strcpy(pkg_dt->domain, "github.com");
		path_ptr = strstr(path_ptr, "github.com") + 11;
	} else if (strstr(path_ptr, "gitlab.com")) {
		strcpy(pkg_dt->host, "gitlab");
		strcpy(pkg_dt->domain, "gitlab.com");
		path_ptr = strstr(path_ptr, "gitlab.com") + 11;
	} else if (strstr(path_ptr, "gitea.com") || strstr(path_ptr, "gitea")) {
		strcpy(pkg_dt->host, "gitea");
		strcpy(pkg_dt->domain, "gitea.com");
		path_ptr = strstr(path_ptr, "/") + 1;
	} else if (strstr(path_ptr, "sourceforge.net")) {
		strcpy(pkg_dt->host, "sourceforge");
		strcpy(pkg_dt->domain, "sourceforge.net");
		path_ptr = strstr(path_ptr, "sourceforge.net") + 16;
	} else {
		printf("\n" DOG_COL_BCYAN "Host not detected clearly for: %s"
		    DOG_COL_DEFAULT "\n", input);
		printf(DOG_COL_BCYAN "1. GitHub\n2. GitLab\n3. Gitea\n4. SourceForge\n");
		choice = readline("Please select host (1-4): ");
		if (choice) {
			if (choice[0] == '1') {
				strcpy(pkg_dt->host, "github");
				strcpy(pkg_dt->domain, "github.com");
			} else if (choice[0] == '2') {
				strcpy(pkg_dt->host, "gitlab");
				strcpy(pkg_dt->domain, "gitlab.com");
			} else if (choice[0] == '3') {
				strcpy(pkg_dt->host, "gitea");
				strcpy(pkg_dt->domain, "gitea.com");
			} else if (choice[0] == '4') {
				strcpy(pkg_dt->host, "sourceforge");
				strcpy(pkg_dt->domain, "sourceforge.net");
			} else {
				strcpy(pkg_dt->host, "github");
				strcpy(pkg_dt->domain, "github.com");
			}
			free(choice);
		}
	}

	slash = strchr(path_ptr, '/');
	if (slash) {
		*slash = '\0';
		strncpy(pkg_dt->user, path_ptr,
		    sizeof(pkg_dt->user) - 1);
		repo_ptr = slash + 1;
		dot_git = strstr(repo_ptr, ".git");
		if (dot_git)
			*dot_git = '\0';
		strncpy(pkg_dt->repo, repo_ptr,
		    sizeof(pkg_dt->repo) - 1);
		dog_free(parse_input);
		return (1);
	}

	dog_free(parse_input);
	return (0);
}

static int
package_gh_release_assets(const char *user, const char *repo, const char *tag,
    char **out_urls, int max_urls)
{
	char		 api_url[DOG_PATH_MAX * 2];
	char		*json_data = NULL;
	const char	*p;
	int		 url_count = REPLICATE_RATE_ZERO;

	snprintf(api_url, sizeof(api_url),
	    "%sapi.github.com/repos/%s/%s/releases/tags/%s",
	    "https://", user, repo, tag);

	if (!package_http_get_content(api_url, dogconfig.dog_toml_github_tokens,
	    &json_data)) {
		dog_free(json_data);
		return (0);
	}

	p = json_data;
	while (url_count < max_urls &&
	    (p = strstr(p, "\"browser_download_url\"")) != NULL) {
		const char	*url_end;
		size_t		 url_len;

		p += strlen("\"browser_download_url\"");
		p = strchr(p, '"');
		if (!p)
			break;
		++p;

		url_end = strchr(p, '"');
		if (!url_end)
			break;

		url_len = url_end - p;
		out_urls[url_count] = dog_malloc(url_len + 1);
		if (!out_urls[url_count])
			unit_ret_main(NULL);

		strncpy(out_urls[url_count], p, url_len);
		out_urls[url_count][url_len] = '\0';

		++url_count;
		p = url_end + 1;
	}

	dog_free(json_data);
	return (url_count);
}

static void
package_build_repo_url(const struct _repositories *pkg_dt,
    int rate_tag_page, char *put_url, size_t put_size)
{
	char	package_actual_tag[128] = {0};

	if (pkg_dt->tag[0]) {
		strncpy(package_actual_tag, pkg_dt->tag,
		    sizeof(package_actual_tag) - 1);

		if (strcmp(package_actual_tag, "newer") == REPLICATE_RATE_ZERO) {
			if (strcmp(pkg_dt->host, "github") ==
			    REPLICATE_RATE_ZERO && !rate_tag_page)
				strcpy(package_actual_tag, "latest");
		}
	}

	if (strcmp(pkg_dt->host, "github") == REPLICATE_RATE_ZERO) {
		if (rate_tag_page && package_actual_tag[0]) {
			if (!strcmp(package_actual_tag, "latest")) {
				snprintf(put_url, put_size,
				    "https://%s/%s/%s/releases/latest",
				    pkg_dt->domain, pkg_dt->user,
				    pkg_dt->repo);
			} else {
				snprintf(put_url, put_size,
				    "https://%s/%s/%s/releases/tag/%s",
				    pkg_dt->domain, pkg_dt->user,
				    pkg_dt->repo, package_actual_tag);
			}
		} else if (package_actual_tag[0]) {
			if (!strcmp(package_actual_tag, "latest")) {
				snprintf(put_url, put_size,
				    "https://%s/%s/%s/releases/latest",
				    pkg_dt->domain, pkg_dt->user,
				    pkg_dt->repo);
			} else {
				snprintf(put_url, put_size,
				    "https://%s/%s/%s/archive/refs/tags/%s.tar.gz",
				    pkg_dt->domain, pkg_dt->user,
				    pkg_dt->repo, package_actual_tag);
			}
		} else {
			snprintf(put_url, put_size,
			    "https://%s/%s/%s/archive/refs/heads/main.zip",
			    pkg_dt->domain, pkg_dt->user,
			    pkg_dt->repo);
		}
	}
}

static int
package_gh_latest_tag(const char *user, const char *repo, char *out_tag,
    size_t put_size)
{
	char		 api_url[DOG_PATH_MAX * 2];
	char		*json_data = NULL;
	const char	*p;
	int		 ret = REPLICATE_RATE_ZERO;

	snprintf(api_url, sizeof(api_url),
	    "%sapi.github.com/repos/%s/%s/releases/latest",
	    "https://", user, repo);

	if (!package_http_get_content(api_url, dogconfig.dog_toml_github_tokens,
	    &json_data))
		return (0);

	p = strstr(json_data, "\"tag_name\"");
	if (p) {
		p = strchr(p, ':');
		if (p) {
			++p;
			while (*p && (*p == ' ' || *p == '\t' || *p == '\n' ||
			    *p == '\r'))
				++p;

			if (*p == '"') {
				++p;
				const char	*end = strchr(p, '"');
				if (end) {
					size_t	 tag_len = end - p;
					if (tag_len < put_size) {
						strncpy(out_tag, p, tag_len);
						out_tag[tag_len] = '\0';
						ret = 1;
					}
				}
			}
		}
	}

	dog_free(json_data);
	return (ret);
}

static int
try_handle_gen_repo(const struct _repositories *repo, char *put_url,
    size_t put_size, const char *branch)
{
	if (strcmp(repo->host, "gitlab") == 0) {
		snprintf(put_url, put_size,
		    "https://gitlab.com/%s/%s/-/archive/%s/%s-%s.tar.gz",
		    repo->user, repo->repo, branch ? branch : "main",
		    repo->repo, branch ? branch : "main");
		return (1);
	} else if (strcmp(repo->host, "gitea") == 0) {
		snprintf(put_url, put_size,
		    "https://gitea.com/%s/%s/archive/%s.tar.gz",
		    repo->user, repo->repo, branch ? branch : "main");
		return (1);
	} else if (strcmp(repo->host, "sourceforge") == 0) {
		snprintf(put_url, put_size,
		    "https://sourceforge.net/projects/%s/files/latest/download",
		    repo->repo);
		return (1);
	}
	return (0);
}

static int
package_handle_repo(const struct _repositories *kevlar_repos, char *put_url,
    size_t put_size, const char *branch)
{
	const char	*package_repo_branch[] = { branch, "main", "master" };
	const char	*opr;
	char		 package_actual_tag[128];
	char		*package_assets[10];
	char		*package_best_asset;
	int		 ret = 0, j, asset_counts, use_fallback_branch = 0;

	if (strcmp(kevlar_repos->host, "github") != 0)
		return (try_handle_gen_repo(kevlar_repos, put_url,
		    put_size, branch));

	if (kevlar_repos->tag[0] &&
	    strcmp(kevlar_repos->tag, "newer") == REPLICATE_RATE_ZERO) {
		if (package_gh_latest_tag(kevlar_repos->user,
		    kevlar_repos->repo, package_actual_tag,
		    sizeof(package_actual_tag))) {
			pr_info(stdout,
			    "Creating latest/newer tag: " DOG_COL_CYAN
			    "%s " DOG_COL_DEFAULT "~instead of latest "
			    DOG_COL_CYAN "(?newer)" DOG_COL_DEFAULT "\t\t"
			    DOG_COL_YELLOW "[V]",
			    package_actual_tag);
		} else {
			pr_error(stdout,
			    "Failed to get latest tag for %s/%s,"
			    "Falling back to main branch\t\t[Fail]",
			    kevlar_repos->user, kevlar_repos->repo);
			minimal_debugging();
			use_fallback_branch = 1;
		}
	} else {
		strncpy(package_actual_tag, kevlar_repos->tag,
		    sizeof(package_actual_tag) - 1);
	}

	if (use_fallback_branch) {
		for (j = REPLICATE_RATE_ZERO; j < 3 && !ret; j++) {
			snprintf(put_url, put_size,
			    "https://github.com/%s/%s/archive/refs/heads/%s.zip",
			    kevlar_repos->user, kevlar_repos->repo,
			    package_repo_branch[j]);

			if (package_url_checking(put_url,
			    dogconfig.dog_toml_github_tokens)) {
				ret = 1;
				if (j == 1) {
					printf("Create master branch "
					    "(main branch not found)"
					    "\t\t" DOG_COL_YELLOW "[V]\n");
				}
			}
		}
		return (ret);
	}

	pr_info(stdout, "Fetching any archive from %s..", package_actual_tag);

	if (package_actual_tag[0]) {
		asset_counts = package_gh_release_assets(kevlar_repos->user,
		    kevlar_repos->repo, package_actual_tag,
		    package_assets, 10);

		if (asset_counts > 0) {
#ifdef DOG_WINDOWS
			opr = "windows";
#else
			opr = "linux";
#endif

			package_best_asset = package_fetching_assets(
			    package_assets, asset_counts, opr);

			if (package_best_asset) {
				strncpy(put_url, package_best_asset,
				    put_size - 1);
				ret = 1;

				if (this_more_archive(package_best_asset)) {
					;
				} else if (this_os_archive(package_best_asset)) {
					;
				} else {
					;
				}

				pr_info(stdout,
				    "Trying to install Archive:\n   "
				    DOG_COL_YELLOW "\033[1m > \033[0m"
				    DOG_COL_CYAN "%s\t\t" DOG_COL_YELLOW "[V]\n",
				    package_best_asset);

				dog_free(package_best_asset);
			}

			for (j = REPLICATE_RATE_ZERO; j < asset_counts; j++)
				dog_free(package_assets[j]);
		}

		if (!ret) {
			const char	*package_arch_format[] = {
				"https://github.com/%s/%s/archive/refs/tags/%s.tar.gz",
				"https://github.com/%s/%s/archive/refs/tags/%s.zip"
			};

			for (j = REPLICATE_RATE_ZERO; j < 2 && !ret; j++) {
				snprintf(put_url, put_size,
				    package_arch_format[j],
				    kevlar_repos->user,
				    kevlar_repos->repo,
				    package_actual_tag);

				if (package_url_checking(put_url,
				    dogconfig.dog_toml_github_tokens))
					ret = 1;
			}
		}
	} else {
		for (j = REPLICATE_RATE_ZERO; j < 2 && !ret; j++) {
			snprintf(put_url, put_size,
			    "%s%s/%s/archive/refs/heads/%s.zip",
			    "https://github.com/",
			    kevlar_repos->user,
			    kevlar_repos->repo,
			    package_repo_branch[j]);

			if (package_url_checking(put_url,
			    dogconfig.dog_toml_github_tokens)) {
				ret = 1;
				if (j == 1)
					printf("Create master branch "
					    "(main branch not found)\t\t"
					    DOG_COL_YELLOW "[V]\n");
			}
		}
	}

	return (ret);
}

int
package_try_parsing(const char *raw_file_path, const char *raw_json_path)
{
	char	res_convert_f_path[DOG_PATH_MAX],
		res_convert_json_path[DOG_PATH_MAX];

	strncpy(res_convert_f_path, raw_file_path,
	    sizeof(res_convert_f_path));
	res_convert_f_path[sizeof(res_convert_f_path) - 1] = '\0';
	path_sym_convert(res_convert_f_path);

	strncpy(res_convert_json_path, raw_json_path,
	    sizeof(res_convert_json_path));
	res_convert_json_path[sizeof(res_convert_json_path) - 1] = '\0';
	path_sym_convert(res_convert_json_path);

	if (strfind(res_convert_json_path, "pawno", true) ||
	    strfind(res_convert_json_path, "qawno", true))
		goto done;

done:
	return (1);
}

void
package_implementation_samp_conf(const char *config_file, const char *fw_line,
    const char *plugin_name)
{
	FILE	*temp_file, *ctr_file;
	char	 temp_path[DOG_PATH_MAX];
	char	 ctr_line[DOG_PATH_MAX];
	int	 int_random_size, t_exist, tr_exist, tr_ln_has_tx;

	srand((unsigned int)time(NULL) ^ rand());
	int_random_size = rand() % 10000000;

	snprintf(temp_path, sizeof(temp_path),
	    ".watchdogs/000%07d_temp", int_random_size);

	temp_file = fopen(temp_path, "w");

	if (dog_server_env() != 1)
		return;

	if (dir_exists(".watchdogs") == REPLICATE_RATE_ZERO)
		MKDIR(".watchdogs");

	pr_color(stdout, DOG_COL_GREEN,
	    "Create Dependencies '%s' into '%s'\t\t" DOG_COL_YELLOW "[V]\n",
	    plugin_name, config_file);

	ctr_file = fopen(config_file, "r");
	if (ctr_file) {
		t_exist = 0;
		tr_exist = 0;
		tr_ln_has_tx = 0;

		while (fgets(ctr_line, sizeof(ctr_line), ctr_file)) {
			ctr_line[strcspn(ctr_line, "\n")] = REPLICATE_RATE_ZERO;
			if (strstr(ctr_line, plugin_name) != NULL)
				t_exist = 1;
			if (strstr(ctr_line, fw_line) != NULL) {
				tr_exist = 1;
				if (strstr(ctr_line, plugin_name) != NULL)
					tr_ln_has_tx = 1;
			}
		}
		fclose(ctr_file);

		if (t_exist)
			return;

		if (tr_exist && !tr_ln_has_tx) {
			ctr_file = fopen(config_file, "r");

			while (fgets(ctr_line, sizeof(ctr_line), ctr_file)) {
				char	clean_line[DOG_PATH_MAX];
				strcpy(clean_line, ctr_line);
				clean_line[strcspn(clean_line, "\n")] =
				    REPLICATE_RATE_ZERO;

				if (strstr(clean_line, fw_line) != NULL &&
				    strstr(clean_line, plugin_name) == NULL) {
					fprintf(temp_file,
					    "%s %s\n", clean_line, plugin_name);
				} else {
					fputs(ctr_line, temp_file);
				}
			}

			fclose(ctr_file);
			fclose(temp_file);

			remove(config_file);
			rename(".watchdogs/depends_tmp", config_file);
		} else if (!tr_exist) {
			ctr_file = fopen(config_file, "a");
			fprintf(ctr_file, "%s %s\n", fw_line, plugin_name);
			fclose(ctr_file);
		}
	} else {
		ctr_file = fopen(config_file, "w");
		fprintf(ctr_file, "%s %s\n", fw_line, plugin_name);
		fclose(ctr_file);
	}

	return;
}

#define S_ADD_PLUGIN(config_file, fw_line, plugin_name) \
	package_implementation_samp_conf(config_file, fw_line, plugin_name)

void
package_implementation_omp_conf(const char *config_name,
    const char *package_name)
{
	FILE	*ctr_file;
	cJSON	*s_root, *pawn, *leg, *dir_item, *size_package_name;
	char	*buffer, *__printed;
	long	 fle_size;
	size_t	 file_read;
	int	 p_exist;

	if (dog_server_env() != 2)
		return;

	pr_color(stdout, DOG_COL_GREEN,
	    "Create Dependencies '%s' into '%s'\t\t" DOG_COL_YELLOW "[V]\n",
	    package_name, config_name);

	ctr_file = fopen(config_name, "r");

	if (!ctr_file) {
		s_root = cJSON_CreateObject();
	} else {
		fseek(ctr_file, REPLICATE_RATE_ZERO, SEEK_END);
		fle_size = ftell(ctr_file);
		fseek(ctr_file, REPLICATE_RATE_ZERO, SEEK_SET);

		buffer = (char *)dog_malloc(fle_size + 1);
		if (!buffer) {
			pr_error(stdout, "Memory allocation failed!");
			minimal_debugging();
			fclose(ctr_file);
			return;
		}

		file_read = fread(buffer, 1, fle_size, ctr_file);
		if (file_read != fle_size) {
			pr_error(stdout, "Failed to read the entire file!");
			minimal_debugging();
			dog_free(buffer);
			fclose(ctr_file);
			return;
		}

		buffer[fle_size] = '\0';
		fclose(ctr_file);

		s_root = cJSON_Parse(buffer);
		dog_free(buffer);

		if (!s_root)
			s_root = cJSON_CreateObject();
	}

	pawn = cJSON_GetObjectItem(s_root, "pawn");
	if (!pawn) {
		pawn = cJSON_CreateObject();
		cJSON_AddItemToObject(s_root, "pawn", pawn);
	}

	leg = cJSON_GetObjectItem(pawn, "legacy_plugins");
	if (!leg) {
		leg = cJSON_CreateArray();
		cJSON_AddItemToObject(pawn, "legacy_plugins", leg);
	}

	if (!cJSON_IsArray(leg)) {
		cJSON_DeleteItemFromObject(pawn, "legacy_plugins");
		leg = cJSON_CreateArray();
		cJSON_AddItemToObject(pawn, "legacy_plugins", leg);
	}

	p_exist = 0;
	cJSON_ArrayForEach(dir_item, leg) {
		if (cJSON_IsString(dir_item) &&
		    !strcmp(dir_item->valuestring, package_name)) {
			p_exist = 1;
			break;
		}
	}

	if (!p_exist) {
		size_package_name = cJSON_CreateString(package_name);
		cJSON_AddItemToArray(leg, size_package_name);
	}

	__printed = cJSON_Print(s_root);
	ctr_file = fopen(config_name, "w");
	if (ctr_file) {
		fputs(__printed, ctr_file);
		fclose(ctr_file);
	}

	cJSON_Delete(s_root);
	dog_free(__printed);

	return;
}

#define M_ADD_PLUGIN(x, y) package_implementation_omp_conf(x, y)

void
package_add_include(const char *modes, char *package_name,
    char *package_following)
{
	FILE	*m_file, *n_file;
	char	*ct_modes, *pos, *insert_at, *line_start, *line_end;
	char	*last_inc, *last_inc_end, *end;
	char	 lgth_name[DOG_PATH_MAX];
	long	 fle_size;
	size_t	 bytes_read;
	int	 len;

	if (path_exists(modes) == REPLICATE_RATE_ZERO)
		return;

	m_file = fopen(modes, "rb");
	if (!m_file)
		return;

	fseek(m_file, REPLICATE_RATE_ZERO, SEEK_END);
	fle_size = ftell(m_file);
	fseek(m_file, REPLICATE_RATE_ZERO, SEEK_SET);

	ct_modes = dog_malloc(fle_size + 2);
	if (!ct_modes) {
		fclose(m_file);
		return;
	}

	bytes_read = fread(ct_modes, 1, fle_size, m_file);
	if (bytes_read != fle_size) {
		pr_error(stdout, "Failed to read the entire file!");
		minimal_debugging();
		dog_free(ct_modes);
		fclose(m_file);
		return;
	}

	ct_modes[fle_size] = '\0';
	fclose(m_file);

	strncpy(lgth_name, package_name, sizeof(lgth_name) - 1);

	if (strchr(lgth_name, '<') && strchr(lgth_name, '>')) {
		char	*open = strchr(lgth_name, '<');
		char	*close = strchr(lgth_name, '>');
		if (open && close) {
			memmove(lgth_name, open + 1, close - open - 1);
			lgth_name[close - open - 1] = '\0';
		}
	}

	pos = ct_modes;
	while ((pos = strstr(pos, "#include"))) {
		line_end = strchr(pos, '\n');
		if (!line_end)
			line_end = ct_modes + fle_size;

		len = line_end - pos;
		if (len >= 256)
			len = 255;
		char	 line[256];
		strncpy(line, pos, len);
		line[len] = '\0';

		if (strstr(line, lgth_name)) {
			dog_free(ct_modes);
			return;
		}
		pos = line_end + 1;
	}

	insert_at = NULL;
	pos = ct_modes;

	while ((pos = strstr(pos, package_following))) {
		line_start = pos;
		while (line_start > ct_modes && *(line_start - 1) != '\n')
			line_start--;

		line_end = strchr(pos, '\n');
		if (!line_end)
			line_end = ct_modes + fle_size;

		len = line_end - line_start;
		if (len >= 256)
			len = 255;
		char	 line[256];
		strncpy(line, line_start, len);
		line[len] = '\0';

		if (strstr(line, package_following)) {
			insert_at = line_end;
			break;
		}
		pos = line_end + 1;
	}

	n_file = fopen(modes, "w");
	if (!n_file) {
		dog_free(ct_modes);
		return;
	}

	if (insert_at) {
		fwrite(ct_modes, 1, insert_at - ct_modes, n_file);

		if (insert_at > ct_modes && *(insert_at - 1) != '\n')
			fprintf(n_file, "\n");

		fprintf(n_file, "%s\n", package_name);

		if (*insert_at) {
			end = ct_modes + fle_size;
			while (end > insert_at && (*(end - 1) == '\n' ||
			    *(end - 1) == '\r' || *(end - 1) == ' ' ||
			    *(end - 1) == '\t'))
				end--;

			if (end > insert_at)
				fwrite(insert_at, 1, end - insert_at, n_file);
			fprintf(n_file, "\n");
		}
	} else {
		last_inc = NULL;
		last_inc_end = NULL;
		pos = ct_modes;

		while ((pos = strstr(pos, "#include"))) {
			last_inc = pos;
			last_inc_end = strchr(pos, '\n');
			if (!last_inc_end)
				last_inc_end = ct_modes + fle_size;
			pos = last_inc_end + 1;
		}

		if (last_inc_end) {
			fwrite(ct_modes, 1, last_inc_end - ct_modes, n_file);
			fprintf(n_file, "\n%s\n", package_name);

			end = ct_modes + fle_size;
			while (end > last_inc_end && (*(end - 1) == '\n' ||
			    *(end - 1) == '\r'))
				end--;

			if (end > last_inc_end)
				fwrite(last_inc_end, 1, end - last_inc_end,
				    n_file);
		} else {
			fprintf(n_file, "%s\n", package_name);
			fwrite(ct_modes, 1, fle_size, n_file);
		}
	}

	fclose(n_file);
	dog_free(ct_modes);
}

#define DENCY_ADD_INCLUDES(x, y, z) package_add_include(x, y, z)

static void
package_include_prints(const char *package_include)
{
	toml_table_t	*dog_toml_config;
	FILE		*this_proc_file;
	char		 dog_buffer_error[DOG_PATH_MAX],
			 dependencies[DOG_PATH_MAX], _directive[DOG_MAX_PATH];
	const char	*package_n, *direct_bnames;
	char		*userinput;
	static int	 k = 0;
	static const char *expect_add;

	package_n = try_get_filename(package_include);
	snprintf(dependencies, sizeof(dependencies), "%s", package_n);
	direct_bnames = try_get_basename(dependencies);

	this_proc_file = fopen("watchdogs.toml", "r");
	if (this_proc_file) {
		dog_toml_config = toml_parse_file(this_proc_file,
		    dog_buffer_error, sizeof(dog_buffer_error));
		fclose(this_proc_file);

		if (!dog_toml_config) {
			pr_error(stdout,
			    "failed to parse the watchdogs.toml..: %s",
			    dog_buffer_error);
			minimal_debugging();
			return;
		}

		toml_table_t *dog_compiler = toml_table_in(dog_toml_config,
		    "compiler");
		if (dog_compiler) {
			toml_datum_t toml_proj_i = toml_string_in(dog_compiler,
			    "input");
			if (toml_proj_i.ok) {
				dogconfig.dog_toml_proj_input =
				    strdup(toml_proj_i.u.s);
				dog_free(toml_proj_i.u.s);
			}
		}
		toml_free(dog_toml_config);
	}

	snprintf(_directive, sizeof(_directive), "#include <%s>", direct_bnames);

	if (k == 0) {
		printf(DOG_COL_BCYAN
		    "Where do you want to install %s? (enter for: %s)"
		    DOG_COL_DEFAULT, _directive,
		    dogconfig.dog_toml_proj_input);
		fflush(stdout);
		userinput = readline(" ");
		if (userinput[0] == '\0')
			userinput = strdup(dogconfig.dog_toml_proj_input);
		expect_add = strdup(userinput);
		dog_free(userinput);
		++k;
	}

	char	size_userinput[DOG_PATH_MAX] = {0};
	snprintf(size_userinput, sizeof(size_userinput), "gamemodes/%s.pwn",
	    expect_add);

	if (path_exists(expect_add) == 0) {
		FILE	*creat = fopen(size_userinput, "w+");
		if (creat)
			fclose(creat);
	}

	expect_add = strdup(size_userinput);

	if (dog_server_env() == 1) {
		DENCY_ADD_INCLUDES(expect_add, _directive, "#include <a_samp>");
	} else if (dog_server_env() == 2) {
		DENCY_ADD_INCLUDES(expect_add, _directive, "#include <open.mp>");
	} else {
		DENCY_ADD_INCLUDES(expect_add, _directive, "#include <a_samp>");
	}
}

void
dump_file_type(const char *dump_path, char *dump_pattern,
    char *dump_exclude, char *dump_loc, char *dump_place, int dump_root)
{
    const char *package_names, *basename, *match_root_keywords;
    char *basename_lwr;
    int i, found, rate_has_prefix;

    dog_sef_restore();

    found = dog_sef_fdir(dump_path, dump_pattern, dump_exclude);
    ++fdir_counts;
#if defined(_DBG_PRINT)
    println(stdout, "fdir_counts (%d): %d", fdir_counts, found);
#endif

    if (found) {
        for (i = REPLICATE_RATE_ZERO; i < dogconfig.dog_sef_count; ++i) {
            package_names = try_get_filename(
                dogconfig.dog_sef_found_list[i]);
            basename = try_get_basename(
                dogconfig.dog_sef_found_list[i]);

            basename_lwr = strdup(basename);
            for (int j = REPLICATE_RATE_ZERO; basename_lwr[j]; j++)
                basename_lwr[j] = tolower(basename_lwr[j]);

            rate_has_prefix = REPLICATE_RATE_ZERO;

            match_root_keywords = dogconfig.dog_toml_root_patterns;
            while (*match_root_keywords) {
                while (*match_root_keywords == ' ')
                    match_root_keywords++;

                const char *end = match_root_keywords;
                while (*end && *end != ' ')
                    end++;

                if (end > match_root_keywords) {
                    size_t keyword_len = end - match_root_keywords;
                    if (strncmp(basename_lwr,
                        match_root_keywords,
                        keyword_len) == REPLICATE_RATE_ZERO)
                    {
                        ++rate_has_prefix;
                        break;
                    }
                }

                match_root_keywords = (*end) ? end + 1 : end;
            }
            dog_free(basename_lwr);

#ifdef DOG_WINDOWS
            char *separator = "\\";
#else
            char *separator = "/";
#endif

            char dest_path[DOG_PATH_MAX];
            dest_path[0] = '\0';

            if (dump_place[0] != '\0') {
                snprintf(dest_path, sizeof(dest_path), "%s%s%s%s%s",
                         dump_loc, separator, dump_place, separator,
                         package_names);

                char dir_part[DOG_PATH_MAX];
                snprintf(dir_part, sizeof(dir_part), "%s%s%s",
                         dump_loc, separator, dump_place);
                if (dir_exists(dir_part) == REPLICATE_RATE_ZERO) {
                    dog_mkdir(dir_part);
                }
            } else if (rate_has_prefix) {
                snprintf(dest_path, sizeof(dest_path), "%s%s%s",
                         dump_loc, separator, package_names);
            } else {
                snprintf(dest_path, sizeof(dest_path), "%s%s%s%s",
                         dump_loc, separator, separator, package_names);

                char plugins_dir[DOG_PATH_MAX];
                snprintf(plugins_dir, sizeof(plugins_dir), "%s%s",
                         dump_loc, separator);
                if (dir_exists(plugins_dir) == REPLICATE_RATE_ZERO) {
                    dog_mkdir(plugins_dir);
                }
            }

            int move_success = 0;
#ifdef DOG_WINDOWS
            if (MoveFileExA(dogconfig.dog_sef_found_list[i], dest_path,
                           MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                move_success = 1;
            } else {
                char cmd[DOG_PATH_MAX * 3];
                snprintf(cmd, sizeof(cmd), "copy /Y \"%s\" \"%s\" >nul 2>&1 && del \"%s\"",
                         dogconfig.dog_sef_found_list[i], dest_path,
                         dogconfig.dog_sef_found_list[i]);
                if (system(cmd) == 0) {
                    move_success = 1;
                }
            }
#else
            if (rename(dogconfig.dog_sef_found_list[i], dest_path) == 0) {
                move_success = 1;
            } else {
                pid_t pid = fork();
                if (pid == 0) {
                    execlp("mv", "mv", "-f",
                           dogconfig.dog_sef_found_list[i],
                           dest_path,
                           NULL);
                    _exit(127);
                }
                waitpid(pid, NULL, 0);
                move_success = 1;
            }
#endif
            if (move_success) {
                if (dump_place[0] != '\0') {
                    pr_color(stdout, DOG_COL_CYAN,
                        " [M] Plugins %s -> %s%s%s\n",
                        basename, dump_loc, separator, dump_place);
                } else if (rate_has_prefix) {
                    pr_color(stdout, DOG_COL_CYAN,
                        " [M] Plugins %s -> %s\n",
                        basename, dump_loc);
                } else {
                    pr_color(stdout, DOG_COL_CYAN,
                        " [M] Plugins %s -> %s%s?\n",
                        basename, dump_loc, separator);
                }
            } else {
                pr_error(stdout, "Failed to move: %s", basename);
            }

            snprintf(json_item, sizeof(json_item), "%s",
                package_names);
            package_try_parsing(json_item, json_item);

            if (dump_root == 1)
                goto done;

            if (dog_server_env() == 1 &&
                strfind(dogconfig.dog_toml_config,
                ".cfg", true))
                S_ADD_PLUGIN(dogconfig.dog_toml_config,
                    "plugins", basename);
            else if (dog_server_env() == 2 &&
                strfind(dogconfig.dog_toml_config,
                ".json", true))
                M_ADD_PLUGIN(dogconfig.dog_toml_config,
                    basename);
        }
    }

done:
    return;
}

void
package_cjson_additem(cJSON *p1, int p2, cJSON *p3)
{
	if (cJSON_IsString(cJSON_GetArrayItem(p1, p2)))
		cJSON_AddItemToArray(p3,
		    cJSON_CreateString(cJSON_GetArrayItem(p1, p2)->valuestring));
}

void
package_move_files(const char *package_dir, const char *package_loc)
{
    struct dirent *dir_item;
    struct stat dir_st;
    DIR     *open_dir;
    char    **tmp_stack, **stack, *pos, *include_path, *src_sep;
    char    root_dir[DOG_PATH_MAX], includes[DOG_PATH_MAX],
            plugins[DOG_PATH_MAX], components[DOG_PATH_MAX],
            the_path[DOG_PATH_MAX * 2], full_path[DOG_MAX_PATH],
            src[DOG_PATH_MAX], dest[DOG_MAX_PATH * 2];
    const char *packages;
    int i, _sindex, _ssize, rate_found_include;

    fdir_counts = REPLICATE_RATE_ZERO;

    _ssize = DOG_MAX_PATH;
    tmp_stack = dog_malloc(_ssize * sizeof(char *));
    if (tmp_stack == REPLICATE_RATE_ZERO)
        return;
    stack = tmp_stack;
    for (i = REPLICATE_RATE_ZERO; i < _ssize; i++) {
        stack[i] = dog_malloc(DOG_PATH_MAX);
        if (stack[i] == REPLICATE_RATE_ZERO) {
            for (int j = REPLICATE_RATE_ZERO; j < i; j++)
                dog_free(stack[j]);
            dog_free(stack);
            return;
        }
    }

#ifdef DOG_WINDOWS
    snprintf(plugins, sizeof(plugins), "%s\\plugins", package_dir);
    snprintf(components, sizeof(components), "%s\\components", package_dir);
    snprintf(includes, sizeof(includes), "pawno\\include");
    char *separator = "\\";
#else
    snprintf(plugins, sizeof(plugins), "%s/plugins", package_dir);
    snprintf(components, sizeof(components), "%s/components", package_dir);
    snprintf(includes, sizeof(includes), "pawno/include");
    char *separator = "/";
#endif

    if (dog_server_env() == 1) {
        if (dog_server_env() == 2) {
#ifdef DOG_WINDOWS
            snprintf(includes, sizeof(includes), "qawno\\include");
#else
            snprintf(includes, sizeof(includes), "qawno/include");
#endif
        }
    }

    snprintf(full_path, sizeof(full_path), "%s%s%s",
             package_dir, separator, includes);

#if defined(_DBG_PRINT)
    println(stdout, "Full include path: %s", full_path);
#endif

    dog_sef_restore();
    rate_found_include = dog_sef_fdir(full_path, "*.inc", NULL);

    if (rate_found_include > 0) {
        for (i = REPLICATE_RATE_ZERO; i < dogconfig.dog_sef_count; ++i) {
            packages = try_get_filename(dogconfig.dog_sef_found_list[i]);
            char dest_path[DOG_MAX_PATH + DOG_PATH_MAX];

            char include_dest[DOG_MAX_PATH];
            snprintf(include_dest, sizeof(include_dest), "%s%s%s",
                     package_loc, separator, includes);

            if (dir_exists(include_dest) == REPLICATE_RATE_ZERO) {
                dog_mkdir(include_dest);
            }

            snprintf(dest_path, sizeof(dest_path), "%s%s%s",
                     include_dest, separator, packages);

#ifdef DOG_WINDOWS
            if (MoveFileExA(dogconfig.dog_sef_found_list[i], dest_path,
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
                pr_color(stdout, DOG_COL_CYAN,
                        " [M] Include %s -> %s\n",
                        dogconfig.dog_sef_found_list[i], dest_path);
            } else {
                pr_error(stdout, "Failed to move include: %s",
                        dogconfig.dog_sef_found_list[i]);
            }
#else
            pid_t pid = fork();
            if (pid == 0) {
                execlp("mv", "mv", "-f",
                       dogconfig.dog_sef_found_list[i],
                       dest_path,
                       NULL);
                _exit(127);
            }
            waitpid(pid, NULL, 0);
            pr_color(stdout, DOG_COL_CYAN,
                    " [M] Include %s -> %s\n",
                    dogconfig.dog_sef_found_list[i], dest_path);
#endif

            package_try_parsing(packages, packages);
            package_include_prints(packages);
        }
    }

    if (dir_exists(plugins)) {
        char *size_location = strdup(package_loc);

        char plugins_dest[DOG_PATH_MAX];
        snprintf(plugins_dest, sizeof(plugins_dest), "%s%splugins",
                 package_loc, separator);

        if (dir_exists(plugins_dest) == REPLICATE_RATE_ZERO) {
            dog_mkdir(plugins_dest);
        }

#ifdef DOG_WINDOWS
        dump_file_type(plugins, "*.dll", NULL, plugins_dest, "", 0);
#else
        dump_file_type(plugins, "*.so", NULL, plugins_dest, "", 0);
#endif

        dog_free(size_location);
    }

    for (i = REPLICATE_RATE_ZERO; i < _ssize; i++)
        dog_free(stack[i]);
    dog_free(stack);

    printf("\n");

	destroy_arch_dir(package_dir);

    return;
}

void
dog_apply_depends(const char *depends_name, const char *depends_location)
{
	char	_dependencies[DOG_PATH_MAX], package_dir[DOG_PATH_MAX];
	char	size_depends_location[DOG_PATH_MAX * 2];
	char	*extension;

	snprintf(_dependencies, sizeof(_dependencies), "%s", depends_name);

#if defined(_DBG_PRINT)
	println(stdout, "_dependencies: %s", _dependencies);
#endif

	if ((extension = strstr(_dependencies, ".tar.gz")) != NULL)
		*extension = '\0';
	else if ((extension = strstr(_dependencies, ".tar")) != NULL)
		*extension = '\0';
	else if ((extension = strstr(_dependencies, ".zip")) != NULL)
		*extension = '\0';

	snprintf(package_dir, sizeof(package_dir), "%s", _dependencies);

#if defined(_DBG_PRINT)
	println(stdout, "dency dir: %s", package_dir);
#endif

	if (dog_server_env() == 1) {
		snprintf(size_depends_location, sizeof(size_depends_location),
		    "%s/pawno/include", depends_location);
		if (dir_exists(size_depends_location) == REPLICATE_RATE_ZERO)
			dog_mkdir(size_depends_location);

		snprintf(size_depends_location, sizeof(size_depends_location),
		    "%s/plugins", depends_location);
		if (dir_exists(size_depends_location) == REPLICATE_RATE_ZERO)
			dog_mkdir(size_depends_location);
	} else if (dog_server_env() == 2) {
		snprintf(size_depends_location, sizeof(size_depends_location),
		    "%s/qawno/include", depends_location);
		if (dir_exists(size_depends_location) == REPLICATE_RATE_ZERO)
			dog_mkdir(size_depends_location);

		snprintf(size_depends_location, sizeof(size_depends_location),
		    "%s/plugins", depends_location);
		if (dir_exists(size_depends_location) == REPLICATE_RATE_ZERO)
			dog_mkdir(size_depends_location);

		snprintf(size_depends_location, sizeof(size_depends_location),
		    "%s/components", depends_location);
		if (dir_exists(size_depends_location) == REPLICATE_RATE_ZERO)
			dog_mkdir(size_depends_location);
	}

	package_move_files(package_dir, depends_location);
}

void
dog_install_depends(const char *packages, const char *branch, const char *where)
{
	char			 buffer[1024], package_url[1024],
				 package_name[DOG_PATH_MAX];
	char			*procure_buffer, *fetch_pwd, *init_location,
				*locations;
	const char		*size_last_slash;
	const char		*dependencies[MAX_DEPENDS];
	struct _repositories	 repo;
	int			 package_counts, i;
	static int		 k = 0;

	memset(dependencies, REPLICATE_RATE_ZERO, sizeof(dependencies));

	dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PACKAGE] = DOG_GARBAGE_ZERO;

	if (!packages || !*packages) {
		pr_color(stdout, DOG_COL_RED, "");
		printf("no valid dependencies to install!\t\t[Fail]\n");
		goto done;
	}

	if (strlen(packages) >= sizeof(buffer)) {
		pr_color(stdout, DOG_COL_RED, "");
		printf("packages too long!\t\t[Fail]\n");
		goto done;
	}

	snprintf(buffer, sizeof(buffer), "%s", packages);

	procure_buffer = strtok(buffer, " ");
	while (procure_buffer && package_counts < MAX_DEPENDS) {
		dependencies[package_counts++] = procure_buffer;
		procure_buffer = strtok(NULL, " ");
	}

	if (!package_counts) {
		pr_color(stdout, DOG_COL_RED, "");
		printf("no valid dependencies to install!\t\t[Fail]\n");
		goto done;
	}

	for (i = REPLICATE_RATE_ZERO; i < package_counts; i++) {
		if (!package_parse_repo(dependencies[i], &repo)) {
			pr_color(stdout, DOG_COL_RED, "");
			printf("invalid repo format: %s\t\t[Fail]\n",
			    dependencies[i]);
			continue;
		}

		if (!strcmp(repo.host, "github")) {
			if (!package_handle_repo(&repo, package_url,
			    sizeof(package_url), branch)) {
				pr_color(stdout, DOG_COL_RED, "");
				printf("repo not found: %s\t\t[Fail]\n",
				    dependencies[i]);
				continue;
			}
		} else {
			package_build_repo_url(&repo, REPLICATE_RATE_ZERO,
			    package_url, sizeof(package_url));
			if (!package_url_checking(package_url,
			    dogconfig.dog_toml_github_tokens)) {
				pr_color(stdout, DOG_COL_RED, "");
				printf("repo not found: %s\t\t[Fail]\n",
				    dependencies[i]);
				continue;
			}
		}

		if (strrchr(package_url, __PATH_CHR_SEP_LINUX) &&
		    *(strrchr(package_url, __PATH_CHR_SEP_LINUX) + 1)) {
			snprintf(package_name, sizeof(package_name), "%s",
			    strrchr(package_url, __PATH_CHR_SEP_LINUX) + 1);

			if (!strend(package_name, ".tar.gz", true) &&
			    !strend(package_name, ".tar", true) &&
			    !strend(package_name, ".zip", true))
				snprintf(package_name + strlen(package_name),
				    sizeof(package_name) - strlen(package_name),
				    ".zip");
		} else {
			snprintf(package_name, sizeof(package_name),
			    "%s.tar.gz", repo.repo);
		}

		if (!*package_name) {
			pr_color(stdout, DOG_COL_RED, "");
			printf("invalid repo name: %s\t\t[Fail]\n",
			    package_url);
			continue;
		}

		dogconfig.dog_garbage_access[DOG_GARBAGE_IN_INSTALLING_PACKAGE] = DOG_GARBAGE_TRUE;

		dog_download_file(package_url, package_name);
		if (where == NULL || where[0] == '\0') {
			static int	k = 0;
			static char	*fetch_pwd = NULL;
			static char	*init_location = NULL;

			if (!k) {
				printf("\n.. LIST OF DIRECTORY\n");

				#ifdef DOG_LINUX
				{
					DIR *d = opendir(".");
					if (d) {
						struct dirent *de;
						struct stat st;
						char path[DOG_PATH_MAX];

						while ((de = readdir(d)) != NULL) {
							if (de->d_name[0] == '.')
								continue;

							snprintf(path, sizeof(path), "%s", de->d_name);
							if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
								printf("%s\n", de->d_name);
							}
						}
						closedir(d);
					}
				}
				#else
				{
					WIN32_FIND_DATAA ffd;
					HANDLE hFind;
					char search[] = "*";

					hFind = FindFirstFileA(search, &ffd);
					if (hFind != INVALID_HANDLE_VALUE) {
						do {
							if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
								strcmp(ffd.cFileName, ".") != 0 &&
								strcmp(ffd.cFileName, "..") != 0) {
								printf("%s\n", ffd.cFileName);
							}
						} while (FindNextFileA(hFind, &ffd));
						FindClose(hFind);
					}
				}
				#endif

				printf("\n");
				fflush(stdout);

				printf(DOG_COL_BCYAN
				    "Where do you want to install %s? "
				    "(enter for: %s)" DOG_COL_DEFAULT,
				    package_name, dog_procure_pwd());
				fflush(stdout);

				locations = readline(" ");
				if (locations[0] == '\0' || locations[0] == '.') {
					fetch_pwd = dog_procure_pwd();
					init_location = strdup(fetch_pwd);
					dog_apply_depends(package_name,
					    fetch_pwd);
					dog_free(locations);
				} else {
					if (dir_exists(locations) == 0)
						dog_mkdir(locations);
					init_location = strdup(locations);
					dog_apply_depends(package_name,
					    locations);
					dog_free(locations);
				}
				++k;
			} else {
				dog_apply_depends(package_name, init_location);
			}
		} else {
			if (dir_exists(where) == 0)
				dog_mkdir(where);
			dog_apply_depends(package_name, where);
		}
	}

done:
	return;
}
