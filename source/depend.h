#ifndef DEPENDENCY_H
#define DEPENDENCY_H

/*
 * This is the maximum dependency in 'watchdogs.toml' that can be read from the 'packages' key,
 * and you need to ensure this capacity is more than sufficient. If you enter 101, it will only read up to 100.
*/
#define MAX_DEPENDS (102)

struct repositories {
        char host[32]  ; /* repo host */
        char domain[64]; /* repo domain */
        char user[128] ; /* repo user */
        char repo[128] ; /* repo name */
        char tag[128]  ; /* repo tag */
};

void wg_install_depends(const char *depends_string,
		const char *dependencies_branch);

#endif
