#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#define MAX_DEPENDS (102)
#define REPLICATE_RATE_ZERO 0
#define BUILD_PATTERNS(...) { __VA_ARGS__, NULL }
#define WITH_CODENAME(codename) codename

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
    
void dog_install_depends(const char *packages, const char *branch, const char *where);

#endif
