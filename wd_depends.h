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

void wd_apply_depends(const char *depends_name);
void wd_install_depends(const char *deps_str);

#endif
