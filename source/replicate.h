/*-
 * Copyright (c) 2026 Watchdogs Team and contributors
 * All rights reserved. under The 2-Clause BSD License
 * See COPYING or https://opensource.org/license/bsd-2-clause
 */

#ifndef DEPENDENCY_H
#define DEPENDENCY_H

#define MAX_DEPENDS (102)
#define ASSERT_PATTERN(...) { __VA_ARGS__, NULL }

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

extern bool installing_package;

void dog_install_depends(const char *packages, const char *branch, const char *where);

#endif
