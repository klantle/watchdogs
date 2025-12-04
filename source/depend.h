#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#define MAX_DEPENDS (102)

struct dency_curl_buffer {
		char *data;
		size_t size;
};
struct dency_repo_info {
		char host[32];
		char domain[64];
		char user[128];
		char repo[128];
		char tag[128];
};
typedef struct {
		const char *dency_config;
		const char *dency_target;
		const char *dency_added;
} dencyconfig;

#define PATH_SEPARATOR(path) \
    ({ \
        const char *sep_linux = strrchr(path, __PATH_CHR_SEP_LINUX); \
        const char *sep_win32 = strrchr(path, __PATH_CHR_SEP_WIN32); \
        (sep_linux && sep_win32) ? ((sep_linux > sep_win32) ? sep_linux : sep_win32) : (sep_linux ? sep_linux : sep_win32); \
    })

void wg_install_depends(const char *depends_string, const char *dependencies_branch);

#endif
