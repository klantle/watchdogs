#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#define MAX_DEPENDS (102)

#define REPLICATE_RATE_ZERO 0

enum {
    host_size = 32,
    domain_size = 64,
    user_size = 128,
    repo_size = 128,
    tag_size = 128
};

struct _repositories {
    char host[host_size]; char domain[domain_size]; char user[user_size]; char repo[repo_size]; char tag[tag_size];
};

#define PATH_SEPARATOR(sep_path) ({ \
    const char *_p = (sep_path); \
    const char *_l = _p ? strrchr(_p, __PATH_CHR_SEP_LINUX) : NULL; \
    const char *_w = _p ? strrchr(_p, __PATH_CHR_SEP_WIN32) : NULL; \
    (_l && _w) ? ((_l > _w) ? _l : _w) : (_l ? _l : _w); \
})

#define PACKAGE_GET_FILENAME(pkg_path) \
    ({ \
        const char *_p = (pkg_path); \
        const char *_sep = PATH_SEPARATOR(_p); \
        _sep ? _sep + 1 : _p; \
    })

#define PACKAGE_GET_BASENAME(pkg_path) \
    ({ \
        const char *_p = (pkg_path); \
        const char *_sep = PATH_SEPARATOR(_p); \
        _sep ? _sep + 1 : _p; \
    })
    
void wg_install_depends(const char *packages, const char *branch, const char *where);

#endif
