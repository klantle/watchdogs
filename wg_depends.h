#ifndef DEPENDS_H
#define DEPENDS_H

#define MAX_DEPENDS (102)

struct dep_curl_buffer {
		char *data;
		size_t size;
};
struct dep_repo_info {
		char host[32];
		char domain[64];
		char user[128];
		char repo[128];
		char tag[128];
};
typedef struct {
		const char *dep_config;
		const char *dep_target;
		const char *dep_added;
} depConfig;

#define PATH_SEPARATOR(path) \
    ({ \
        const char *sep_linux = strrchr(path, __PATH_CHR_SEP_LINUX); \
        const char *sep_win32 = strrchr(path, __PATH_CHR_SEP_WIN32); \
        (sep_linux && sep_win32) ? ((sep_linux > sep_win32) ? sep_linux : sep_win32) : (sep_linux ? sep_linux : sep_win32); \
    })

void wg_install_depends(const char *depends_string);

#endif
